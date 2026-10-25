/* SPDX-License-Identifier: MIT */
#undef NDEBUG /* Tests must check in every build type. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sim_modem.h"
#include "ts_activation.h"
#include "ts_bear9704.h"
#include "ts_internal.h"

#define TOPIC 1234u
#define IMEI SIM_IMEI

static void start_ready(ts_bear9704_t *b, sim_modem_t *m)
{
    ts_platform_t p;
    sim_init(m);
    p = sim_platform(m);
    assert(ts_bear_init(b, &p, sim_on_event, m) == TS_OK);
    assert(ts_bear_start(b) == TS_OK);
    m->booted = true;
    sim_run(b, m, 3000);
}

static void test_helpers(void)
{
    ts_jspr_frame_t f;
    char line[] = "299 messageOriginateSegment {\"topic_id\":7,\"a\":{\"topic_id\":9}}";
    uint32_t v;
    char b64[16];
    const uint8_t bytes[] = {0, 1, 2, 3, 4};

    assert(ts_jspr_parse(line, &f) && f.code == 299u);
    assert(!strcmp(f.target, "messageOriginateSegment"));
    assert(ts_json_u32(f.json, "topic_id", &v) && v == 7u); /* Top level only. */
    assert(ts_crc16((const uint8_t *)"123456789", 9) == 0x31c3u);
    assert(ts_base64_encode(bytes, 5, b64, sizeof b64) && !strcmp(b64, "AAECAwQ="));
}

static void test_command_builder(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ts_platform_t p;
    ts_cmd_t cmd;
    uint8_t big[TS_LINE_CAP];
    sim_init(&m);
    p = sim_platform(&m);
    assert(ts_bear_init(&b, &p, sim_on_event, &m) == TS_OK);

    assert(ts_cmd_begin(&b, &cmd));
    ts_cmd_str(&cmd, "n=");
    ts_cmd_u32(&cmd, 0u);
    ts_cmd_str(&cmd, ",");
    ts_cmd_u32(&cmd, 4294967295u);
    assert(ts_cmd_end(&b, &cmd) && !strncmp(b.tx, "n=0,4294967295", b.tx_len));
    assert(!ts_cmd_begin(&b, &cmd)); /* Still being sent. */

    b.tx_pos = b.tx_len = 0;
    assert(ts_cmd_begin(&b, &cmd));
    memset(big, 0xaa, sizeof big);
    ts_cmd_base64(&cmd, big, sizeof big); /* Cannot fit. */
    assert(!ts_cmd_end(&b, &cmd));

    assert(ts_cmd_begin(&b, &cmd));
    ts_cmd_str(&cmd, "");
    assert(!ts_cmd_end(&b, &cmd)); /* Empty commands are refused. */
}

static void test_names(void)
{
    assert(!strcmp(ts_state_name(TS_READY), "TS_READY"));
    assert(!strcmp(ts_event_name(TS_EVENT_IMEI), "TS_EVENT_IMEI"));
    assert(!strcmp(ts_result_name(TS_ERR_TIMEOUT), "TS_ERR_TIMEOUT"));
    assert(!strcmp(ts_state_name((ts_state_t)99), "TS_STATE_UNKNOWN"));
}

static void test_duty_cycle(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    const uint8_t data[] = {9, 8, 7};
    int cycle;

    start_ready(&b, &m);
    for (cycle = 0; cycle < 3; ++cycle) {
        m.event_count = 0;
        assert(ts_bear_send(&b, TOPIC, data, sizeof data) == TS_OK);
        sim_run(&b, &m, 500);
        assert(sim_saw(&m, TS_EVENT_MO_DELIVERED));
        assert(ts_bear_shutdown(&b) == TS_OK);
        sim_run(&b, &m, 100);
        assert(ts_bear_state(&b) == TS_OFF && sim_saw(&m, TS_EVENT_STOPPED));
        m.event_count = 0;
        assert(ts_bear_start(&b) == TS_OK);
        m.booted = true;
        sim_run(&b, &m, 3000);
        assert(ts_bear_is_ready(&b));
    }
}

static void test_activation(void)
{
    char text[256];
    assert(ts_imei_is_valid(IMEI));
    assert(!ts_imei_is_valid("30023401075337"));
    assert(!ts_imei_is_valid("30023401075337x"));
    assert(!ts_imei_is_valid(NULL));
    assert(ts_activation_text(IMEI, 21, text, sizeof text) > 0);
    assert(strstr(text, IMEI) && strstr(text, "payloadBytes=21") &&
           strstr(text, TS_ACTIVATE_URL));
    assert(ts_activation_text(IMEI, 0, text, sizeof text) > 0);
    assert(strstr(text, TS_OPTIMIZER_URL));
    assert(ts_activation_text("bad", 0, text, sizeof text) == 0);
    assert(ts_activation_text(IMEI, 0, text, 10) == 0);
}

static void test_startup_and_imei(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    start_ready(&b, &m);
    assert(ts_bear_is_ready(&b));
    assert(sim_saw(&m, TS_EVENT_READY) && sim_saw(&m, TS_EVENT_IMEI));
    assert(ts_bear_imei(&b) && !strcmp(ts_bear_imei(&b), IMEI));

    assert(ts_bear_request_signal(&b) == TS_OK);
    sim_run(&b, &m, 50);
    assert(sim_saw(&m, TS_EVENT_SIGNAL) && ts_bear_signal_bars(&b) == 4);
}

static void test_boot_timeout_and_recovery(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ts_platform_t p;
    sim_init(&m);
    p = sim_platform(&m);
    assert(ts_bear_init(&b, &p, sim_on_event, &m) == TS_OK);
    assert(ts_bear_start(&b) == TS_OK);
    sim_run(&b, &m, (int)TS_BOOT_TIMEOUT_MS + 100);
    assert(!ts_bear_is_ready(&b) && sim_saw(&m, TS_EVENT_ERROR));
    sim_run(&b, &m, 5); /* Let the power-down handshake finish. */
    assert(ts_bear_start(&b) == TS_OK); /* Starting again recovers. */
    m.booted = true;
    sim_run(&b, &m, 3000);
    assert(ts_bear_is_ready(&b));
}

static void test_send(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    const uint8_t data[] = {0, 1, 2, 3, 4};
    start_ready(&b, &m);

    assert(ts_bear_send(&b, 10, data, sizeof data) == TS_ERR_CAPACITY); /* Topic < 64. */
    assert(ts_bear_send(&b, TOPIC, NULL, 0) == TS_ERR_ARGUMENT);
    assert(ts_bear_send(&b, TOPIC, data, sizeof data) == TS_OK);
    assert(ts_bear_send(&b, TOPIC, data, sizeof data) == TS_ERR_NOT_READY);
    sim_run(&b, &m, 500);
    assert(sim_saw(&m, TS_EVENT_MO_ACCEPTED) && sim_saw(&m, TS_EVENT_MO_DELIVERED));
    assert(strstr(m.sent_log, "\"data\":\"AAEC")); /* Payload bytes... */
    assert(strstr(m.sent_log, "\"message_length\":7")); /* ...plus a 2-byte CRC. */
    assert(strstr(m.sent_log, "\"segment_length\":7"));
}

static void test_receive(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    uint8_t message[5] = {1, 2, 3, 0, 0}, out[8];
    char b64[16], line[256];
    size_t length = 0;
    uint16_t topic = 0, crc;

    start_ready(&b, &m);
    crc = ts_crc16(message, 3);
    message[3] = (uint8_t)(crc >> 8);
    message[4] = (uint8_t)(crc & 0xffu);
    assert(ts_base64_encode(message, 5, b64, sizeof b64));

    sim_queue(&m, "299 messageTerminate {\"topic_id\":77,\"message_id\":513,"
              "\"message_length_max\":5}\r");
    snprintf(line, sizeof line,
             "299 messageTerminateSegment {\"topic_id\":77,\"message_id\":513,"
             "\"segment_length\":5,\"segment_start\":0,\"data\":\"%s\"}\r", b64);
    sim_queue(&m, line);
    sim_queue(&m, "299 messageTerminateStatus {\"topic_id\":77,\"message_id\":513,"
              "\"final_mt_status\":\"complete\"}\r");
    sim_run(&b, &m, 600);
    assert(sim_saw(&m, TS_EVENT_MT_RECEIVED));
    assert(ts_bear_take_mt(&b, out, 2, &length, &topic) == TS_ERR_CAPACITY);
    assert(ts_bear_take_mt(&b, out, sizeof out, &length, &topic) == TS_OK);
    assert(length == 3 && topic == 77 && !memcmp(out, message, 3));

    /* A corrupted message is dropped. */
    m.event_count = 0;
    message[0] ^= 0xffu;
    assert(ts_base64_encode(message, 5, b64, sizeof b64));
    sim_queue(&m, "299 messageTerminate {\"topic_id\":77,\"message_id\":514,"
              "\"message_length_max\":5}\r");
    snprintf(line, sizeof line,
             "299 messageTerminateSegment {\"topic_id\":77,\"message_id\":514,"
             "\"segment_length\":5,\"segment_start\":0,\"data\":\"%s\"}\r", b64);
    sim_queue(&m, line);
    sim_queue(&m, "299 messageTerminateStatus {\"topic_id\":77,\"message_id\":514,"
              "\"final_mt_status\":\"complete\"}\r");
    sim_run(&b, &m, 600);
    assert(sim_saw(&m, TS_EVENT_MT_DROPPED) && !sim_saw(&m, TS_EVENT_MT_RECEIVED));
}

static void test_power_down_handshake(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    start_ready(&b, &m);
    assert(ts_bear_shutdown(&b) == TS_OK);
    m.hold_booted = true; /* Module is slow to report it has shut down. */
    sim_run(&b, &m, 100);
    assert(!sim_saw(&m, TS_EVENT_STOPPED));
    assert(ts_bear_start(&b) == TS_ERR_BUSY); /* Not until BOOTED falls. */
    m.hold_booted = false;
    m.booted = false;
    sim_run(&b, &m, 5);
    assert(sim_saw(&m, TS_EVENT_STOPPED) && ts_bear_start(&b) == TS_OK);
}

static void test_shutdown(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    start_ready(&b, &m);
    assert(ts_bear_shutdown(&b) == TS_OK);
    sim_run(&b, &m, 100);
    assert(sim_saw(&m, TS_EVENT_STOPPED) && !ts_bear_is_ready(&b));
}

int main(void)
{
    test_helpers();
    test_command_builder();
    test_names();
    test_activation();
    test_startup_and_imei();
    test_boot_timeout_and_recovery();
    test_send();
#if TS_ENABLE_MT
    test_receive();
#endif
    test_shutdown();
    test_power_down_handshake();
    test_duty_cycle();
    puts("PASS: Telemetry Station SDK tests");
    return 0;
}
