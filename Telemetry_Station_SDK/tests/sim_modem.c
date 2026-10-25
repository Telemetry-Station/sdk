/* SPDX-License-Identifier: MIT */
#undef NDEBUG /* The simulator's checks must run in every build type. */
#include "sim_modem.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sim_init(sim_modem_t *sim)
{
    memset(sim, 0, sizeof *sim);
    sim->topic = 1234u;
}

void sim_queue(sim_modem_t *sim, const char *line)
{
    size_t n = strlen(line);
    assert(sim->reply_len + n < sizeof sim->reply);
    memcpy(sim->reply + sim->reply_len, line, n);
    sim->reply_len += n;
}

void sim_override(sim_modem_t *sim, const char *prefix, const char *reply)
{
    assert(sim->rule_count < SIM_MAX_RULES);
    sim->rules[sim->rule_count].prefix = prefix;
    sim->rules[sim->rule_count].reply = reply;
    sim->rule_count++;
}

static void queue_format(sim_modem_t *sim, const char *format, uint32_t a, uint32_t b)
{
    char line[256];
    (void)snprintf(line, sizeof line, format, (unsigned long)a, (unsigned long)b);
    sim_queue(sim, line);
}

static uint32_t number_after(const char *text, const char *key)
{
    const char *p = strstr(text, key);
    return p ? (uint32_t)strtoul(p + strlen(key), NULL, 10) : 0u;
}

/* Answers one complete command line from the driver. */
static void on_command(sim_modem_t *sim)
{
    const char *c = sim->cmd;
    int i;
    for (i = 0; i < sim->rule_count; ++i) {
        if (!strncmp(c, sim->rules[i].prefix, strlen(sim->rules[i].prefix))) {
            sim_queue(sim, sim->rules[i].reply);
            return;
        }
    }
    if (!strncmp(c, "GET apiVersion", 14))
        sim_queue(sim, "200 apiVersion {\"active_version\":{\"major\":1,\"minor\":6,\"patch\":0}}\r");
    else if (!strncmp(c, "PUT apiVersion", 14))
        sim_queue(sim, "200 apiVersion {\"active_version\":{\"major\":1,\"minor\":6,\"patch\":0}}\r");
    else if (!strncmp(c, "GET simConfig", 13))
        sim_queue(sim, "200 simConfig {\"interface\":\"none\"}\r");
    else if (!strncmp(c, "PUT simConfig", 13))
        sim_queue(sim, "200 simConfig {\"interface\":\"internal\"}\r");
    else if (!strncmp(c, "GET operationalState", 20))
        sim_queue(sim, "200 operationalState {\"state\":\"inactive\"}\r");
    else if (!strncmp(c, "PUT operationalState", 20))
        sim_queue(sim, strstr(c, "inactive")
                       ? "200 operationalState {\"state\":\"inactive\"}\r"
                       : "200 operationalState {\"state\":\"active\"}\r");
    else if (!strncmp(c, "GET constellationState", 22))
        sim_queue(sim, "200 constellationState {\"signal_bars\":4}\r");
    else if (!strncmp(c, "GET hwInfo", 10))
        sim_queue(sim, "200 hwInfo {\"imei\":\"" SIM_IMEI "\"}\r");
    else if (!strncmp(c, "PUT messageOriginateSegment", 27))
        queue_format(sim, "299 messageOriginateStatus {\"topic_id\":%lu,\"message_id\":%lu,"
                          "\"final_mo_status\":\"mo_ack_received\"}\r", sim->topic, 9u);
    else if (!strncmp(c, "PUT messageOriginate ", 21)) {
        queue_format(sim, "200 messageOriginate {\"topic_id\":%lu,\"message_id\":%lu,"
                          "\"message_response\":\"message_accepted\"}\r", sim->topic, 9u);
        /* Ask for the whole message in one segment. */
        {
            char line[256];
            (void)snprintf(line, sizeof line,
                "299 messageOriginateSegment {\"topic_id\":%lu,\"message_id\":9,"
                "\"segment_length\":%lu,\"segment_start\":0}\r",
                (unsigned long)sim->topic,
                (unsigned long)number_after(c, "\"message_length\":"));
            sim_queue(sim, line);
        }
    }
}

static int16_t sim_serial_read(void *user, uint8_t *dst, size_t capacity)
{
    sim_modem_t *sim = user;
    if (sim->read_error) return -1;
    if (!capacity || sim->reply_pos >= sim->reply_len) {
        sim->reply_pos = sim->reply_len = 0;
        return 0;
    }
    *dst = (uint8_t)sim->reply[sim->reply_pos++];
    return 1;
}

static int16_t sim_serial_write(void *user, const uint8_t *src, size_t length)
{
    sim_modem_t *sim = user;
    size_t i, n = length > 3 ? 3 : length; /* Partial writes, like a real UART. */
    if (sim->write_error == 1) return -1;
    if (sim->write_error == 2) return (int16_t)(length + 1u);
    for (i = 0; i < n; ++i) {
        size_t used = strlen(sim->sent_log);
        assert(used + 1 < sizeof sim->sent_log && sim->cmd_len + 1 < sizeof sim->cmd);
        sim->sent_log[used] = (char)src[i];
        sim->sent_log[used + 1] = '\0';
        sim->cmd[sim->cmd_len++] = (char)src[i];
        if (src[i] == '\r') {
            sim->cmd[sim->cmd_len] = '\0';
            on_command(sim);
            sim->cmd_len = 0;
        }
    }
    return (int16_t)n;
}

static uint32_t sim_millis(void *user) { return ((sim_modem_t *)user)->now; }
static bool sim_power_start(void *user) { (void)user; return true; }
static void sim_power_stop(void *user)
{
    sim_modem_t *sim = user;
    if (!sim->hold_booted) sim->booted = false;
}
static void sim_uart_enable(void *user, bool on) { (void)user; (void)on; }
static bool sim_boot_ready(void *user) { return ((sim_modem_t *)user)->booted; }

ts_platform_t sim_platform(sim_modem_t *sim)
{
    ts_platform_t port = {0};
    port.user = sim;
    port.serial_read = sim_serial_read;
    port.serial_write = sim_serial_write;
    port.millis = sim_millis;
    port.power_start = sim_power_start;
    port.power_stop = sim_power_stop;
    port.uart_enable = sim_uart_enable;
    port.boot_ready = sim_boot_ready;
    return port;
}

void sim_on_event(void *user, ts_event_t event, int32_t detail)
{
    sim_modem_t *sim = user;
    if (sim->event_count < SIM_MAX_EVENTS) sim->events[sim->event_count++] = event;
    sim->last_detail = detail;
}

void sim_run(ts_bear9704_t *bear, sim_modem_t *sim, int steps)
{
    while (steps--) {
        sim->now += 1;
        ts_bear_poll(bear);
    }
}

bool sim_saw(const sim_modem_t *sim, ts_event_t event)
{
    int i;
    for (i = 0; i < sim->event_count; ++i)
        if (sim->events[i] == event) return true;
    return false;
}
