/* SPDX-License-Identifier: MIT */
/*
 * Scenario tests for the bench checkout (examples/common/bear_checkout.c).
 *
 * Every check the console report shows is driven through each outcome it can
 * have (PASS, WARN, CHECK, SKIP, and WAIT while pending) against the simulated
 * modem, with the BOOTED line, serial link and console all under test control:
 *
 *   BOOTED   low / high then falls / stuck high / never rises / drops mid-cycle
 *   handshake ok / silent / error code / garbage / unknown state
 *   queries  answered / error / silent / 0..5 bars
 *   send     unset topic / delivered / rejected / timeout / network failure
 *   power    clean / refused / silent / BOOTED stuck high / forced after fault
 *   console  colour, compact soak lines, back-pressure and stalls (lossless)
 *
 * Run "test_checkout transcript" to print the first-cycle report as it would
 * appear on the MSP430 console.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bear_checkout.h"
#include "sim_modem.h"
#include "ts_activation.h"
#include "ts_internal.h"

#define TOPIC 1234u
#define SECOND 1000u
#define CYCLE_LIMIT (TS_MO_TIMEOUT_MS + 4u * TS_BOOT_TIMEOUT_MS)

typedef struct rig rig_t;
typedef void (*hook_fn)(rig_t *r);

struct rig {
    sim_modem_t m;
    ts_platform_t sim;
    ts_bear9704_t bear;
    bear_checkout_t ck;
    bc_config_t cfg;

    /* BOOTED behaviour: delays after ENABLE rises/falls; negative = never. */
    long boot_delay, fall_delay;
    bool rise_pending, fall_pending;
    uint32_t rise_at, fall_at;
    bool start_fails;
    int power_starts;

    /* Console: refuse every Nth write, or everything before stall_until. */
    char out[48 * 1024];
    size_t out_len, cycle_out;
    unsigned refuse_every, writes, refused;
    uint32_t stall_until;

    hook_fn hook;
    int hook_state;
};

/* ---- Platform: the simulated modem plus a scripted BOOTED line ---- */

static int16_t rig_read(void *user, uint8_t *dst, size_t capacity)
{
    rig_t *r = user;
    return r->sim.serial_read(r->sim.user, dst, capacity);
}

static int16_t rig_write_serial(void *user, const uint8_t *src, size_t length)
{
    rig_t *r = user;
    return r->sim.serial_write(r->sim.user, src, length);
}

static uint32_t rig_millis(void *user) { return ((rig_t *)user)->m.now; }

static bool rig_power_start(void *user)
{
    rig_t *r = user;
    r->power_starts++;
    if (r->start_fails) return false;
    r->fall_pending = false;
    r->rise_pending = r->boot_delay >= 0;
    r->rise_at = r->m.now + (uint32_t)(r->boot_delay > 0 ? r->boot_delay : 0);
    return true;
}

static void rig_power_stop(void *user)
{
    rig_t *r = user;
    r->rise_pending = false;
    r->fall_pending = r->fall_delay >= 0;
    r->fall_at = r->m.now + (uint32_t)(r->fall_delay > 0 ? r->fall_delay : 0);
    if (r->fall_pending && r->fall_delay == 0) r->m.booted = false;
}

static void rig_uart(void *user, bool on) { (void)user; (void)on; }
static bool rig_booted(void *user) { return ((rig_t *)user)->m.booted; }

/* ---- Console ---- */

static bool rig_console(void *user, const char *text)
{
    rig_t *r = user;
    size_t n = strlen(text);
    r->writes++;
    if (r->m.now < r->stall_until ||
        (r->refuse_every && r->writes % r->refuse_every == 0)) {
        r->refused++;
        return false;
    }
    /* Every write is one whole line that fits the MSP430 console ring. */
    assert(n > 0 && n < BC_LINE_CAP);
    assert(n >= 2 && text[n - 2] == '\r' && text[n - 1] == '\n');
    assert(r->out_len + n < sizeof r->out);
    memcpy(r->out + r->out_len, text, n + 1);
    r->out_len += n;
    return true;
}

static void on_event(void *user, ts_event_t event, int32_t detail)
{
    rig_t *r = user;
    sim_on_event(&r->m, event, detail);
    bear_checkout_on_event(&r->ck, event, detail);
}

static void rig_init(rig_t *r, uint16_t topic)
{
    ts_platform_t port = {0};
    memset(r, 0, sizeof *r);
    sim_init(&r->m);
    r->m.hold_booted = true; /* BOOTED is scripted by rig_power_stop(). */
    r->sim = sim_platform(&r->m);
    r->boot_delay = 1200;
    r->fall_delay = 800;
    port.user = r;
    port.serial_read = rig_read;
    port.serial_write = rig_write_serial;
    port.millis = rig_millis;
    port.power_start = rig_power_start;
    port.power_stop = rig_power_stop;
    port.uart_enable = rig_uart;
    port.boot_ready = rig_booted;
    assert(ts_bear_init(&r->bear, &port, on_event, r) == TS_OK);

    r->cfg.write = rig_console;
    r->cfg.write_user = r;
    r->cfg.board = "Host simulation";
    r->cfg.wiring = "sim ENABLE, sim BOOTED";
    r->cfg.console = "stdout";
    r->cfg.topic = topic;
    r->cfg.interval_ms = 600000u;
    bear_checkout_init(&r->ck, &r->bear, &r->cfg);
}

static void tick(rig_t *r)
{
    r->m.now++;
    if (r->rise_pending && r->m.now >= r->rise_at) {
        r->rise_pending = false;
        r->m.booted = true;
    }
    if (r->fall_pending && r->m.now >= r->fall_at) {
        r->fall_pending = false;
        r->m.booted = false;
    }
    if (r->hook) r->hook(r);
    ts_bear_poll(&r->bear);
}

static const uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};

/* Runs one whole cycle, report included. Returns its duration. */
static uint32_t run_cycle_with(rig_t *r, const uint8_t *data, size_t length)
{
    uint32_t start = r->m.now, n = 0;
    r->m.event_count = 0;
    r->m.sent_log[0] = '\0'; /* Commands of this cycle only. */
    r->cycle_out = r->out_len;
    assert(bear_checkout_begin(&r->ck, r->m.now, data, length));
    assert(bear_checkout_running(&r->ck));
    assert(!bear_checkout_begin(&r->ck, r->m.now, data, length));
    for (;;) {
        tick(r);
        if (bear_checkout_poll(&r->ck, r->m.now)) break;
        assert(++n < CYCLE_LIMIT);
    }
    assert(!bear_checkout_running(&r->ck));
    assert(!bear_checkout_poll(&r->ck, r->m.now)); /* Reports done once. */
    return r->m.now - start;
}

static uint32_t run_cycle(rig_t *r)
{
    return run_cycle_with(r, payload, sizeof payload);
}

static void idle(rig_t *r, uint32_t ms)
{
    while (ms--) tick(r);
}

/* ---- Assertions ---- */

static const char *cycle_text(const rig_t *r) { return r->out + r->cycle_out; }

static bool has(const rig_t *r, const char *text)
{
    return strstr(cycle_text(r), text) != NULL;
}

static unsigned occurrences(const char *haystack, const char *needle)
{
    unsigned n = 0;
    const char *p = haystack;
    while ((p = strstr(p, needle)) != NULL) { ++n; p += strlen(needle); }
    return n;
}

/* "CHECK  Serial link         no reply ..." */
static bool row(const rig_t *r, const char *tag, bc_check_t check, const char *detail)
{
    char key[64];
    const char *p = cycle_text(r);
    snprintf(key, sizeof key, "%-5s  %-21s", tag, bear_checkout_label(check));
    while ((p = strstr(p, key)) != NULL) {
        const char *end = strstr(p, "\r\n");
        const char *hit = detail ? strstr(p, detail) : p;
        if (hit && hit < end) return true;
        p = end;
    }
    return false;
}

static void dump(const rig_t *r)
{
    fputs(cycle_text(r), stderr);
}

#define CHECK(r, cond) do { if (!(cond)) { dump(r); assert(cond); } } while (0)

static void expect(rig_t *r, const bc_status_t want[BC_CHECK_COUNT])
{
    static const char *const tags[] = {"WAIT", "WAIT", "PASS", "WARN", "CHECK", "SKIP"};
    unsigned i;
    for (i = 0; i < BC_CHECK_COUNT; ++i) {
        bc_status_t got = bear_checkout_status(&r->ck, (bc_check_t)i);
        if (got != want[i]) {
            dump(r);
            fprintf(stderr, "check %s: got %s, want %s\n",
                    bear_checkout_label((bc_check_t)i), tags[got], tags[want[i]]);
            assert(got == want[i]);
        }
        /* The verbose report prints exactly one final row per check. */
        if (r->ck.verbose) {
            char key[64];
            snprintf(key, sizeof key, "%-5s  %-21s", tags[got], bear_checkout_label((bc_check_t)i));
            CHECK(r, occurrences(cycle_text(r), key) == 1u);
        }
    }
}

#define P BC_PASS
#define W BC_WARN
#define F BC_FAIL
#define S BC_SKIP

static void expect_verdict(rig_t *r, bc_verdict_t verdict)
{
    static const char *const text[] = {"RESULT   PASS", "RESULT   WARN", "RESULT   CHECK"};
    CHECK(r, bear_checkout_verdict(&r->ck) == verdict);
    if (r->ck.verbose) CHECK(r, has(r, text[verdict]));
    CHECK(r, ts_bear_state(&r->bear) == TS_OFF || ts_bear_state(&r->bear) == TS_FAULT ||
             ts_bear_state(&r->bear) == TS_FAULT_HOLD);
}

static void setup_reply(rig_t *r, const char *prefix, const char *reply)
{
    sim_override(&r->m, prefix, reply);
}

#define SHUTDOWN_CMD "PUT operationalState {\"state\":\"inactive\""

/* ---- Happy paths ---- */

static void test_activation_cycle(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, S, S, P};
    static rig_t r;
    rig_init(&r, 0);
    run_cycle(&r);
    expect(&r, want);
    expect_verdict(&r, BC_CYCLE_PASS);
    CHECK(&r, has(&r, "BEAR Module 9704 checkout"));
    CHECK(&r, has(&r, " Board    Host simulation"));
    CHECK(&r, has(&r, " Topic    not set; send test skipped"));
    CHECK(&r, row(&r, "PASS", BC_MODULE_OFF, "BOOTED low"));
    CHECK(&r, row(&r, "PASS", BC_BOOT, "BOOTED high after 1200 ms"));
    CHECK(&r, row(&r, "PASS", BC_SIM, "internal SIM selected"));
    CHECK(&r, row(&r, "PASS", BC_IMEI, SIM_IMEI));
    CHECK(&r, row(&r, "PASS", BC_SIGNAL, "4/5 bars"));
    CHECK(&r, row(&r, "SKIP", BC_SEND, "TS_TOPIC_ID not set"));
    CHECK(&r, row(&r, "SKIP", BC_DELIVERY, "nothing sent"));
    CHECK(&r, row(&r, "PASS", BC_SHUTDOWN, "BOOTED low after"));
    CHECK(&r, has(&r, "8 pass  0 warn  0 check  2 skip"));
    CHECK(&r, has(&r, "Next cycle in 600 s (1/1 cycles passed)"));
    CHECK(&r, has(&r, "Activate IMEI " SIM_IMEI " at " TS_ACTIVATE_URL));
    CHECK(&r, !has(&r, "\x1b"));
    CHECK(&r, !bear_checkout_sent(&r.ck));
    CHECK(&r, !strstr(r.m.sent_log, "messageOriginate"));
}

static void test_send_delivered(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, P, P, P};
    static rig_t r;
    rig_init(&r, TOPIC);
    run_cycle(&r);
    expect(&r, want);
    expect_verdict(&r, BC_CYCLE_PASS);
    CHECK(&r, bear_checkout_sent(&r.ck));
    CHECK(&r, row(&r, "PASS", BC_SEND, "accepted as message 9, 5 bytes"));
    CHECK(&r, row(&r, "PASS", BC_DELIVERY, "confirmed by Iridium in"));
    CHECK(&r, has(&r, " Topic    1234"));
    CHECK(&r, !has(&r, "Next steps"));
}

static void test_already_configured_module(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, P, P, P};
    static rig_t r;
    rig_init(&r, TOPIC);
    setup_reply(&r, "GET simConfig", "200 simConfig {\"interface\":\"internal\"}\r");
    setup_reply(&r, "GET operationalState", "200 operationalState {\"state\":\"active\"}\r");
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, !strstr(r.m.sent_log, "PUT simConfig"));
    CHECK(&r, !strstr(r.m.sent_log, "PUT operationalState {\"state\":\"active\""));
}

static void test_api_version_selected(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, S, S, P};
    static rig_t r;
    rig_init(&r, 0);
    setup_reply(&r, "GET apiVersion",
                "200 apiVersion {\"supported_versions\":[{\"major\":1,\"minor\":6,\"patch\":0}]}\r");
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, strstr(r.m.sent_log, "PUT apiVersion") != NULL);
}

/* ---- BOOTED at reset ---- */

static void booted_falls_after_2s(rig_t *r)
{
    if (r->m.now == 2000u) r->m.booted = false;
}

static void test_booted_high_then_falls(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, S, S, P};
    static rig_t r;
    rig_init(&r, 0);
    r.m.booted = true; /* Previous session still shutting down. */
    r.hook = booted_falls_after_2s;
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "PASS", BC_MODULE_OFF, "BOOTED fell after 2000 ms"));
}

static void test_booted_stuck_high(void)
{
    static const bc_status_t want[] = {F, S, S, S, S, S, S, S, S, S};
    static rig_t r;
    uint32_t took;
    rig_init(&r, TOPIC);
    r.m.booted = true;
    took = run_cycle(&r);
    expect(&r, want);
    expect_verdict(&r, BC_CYCLE_FAIL);
    CHECK(&r, took >= TS_BOOT_TIMEOUT_MS);
    CHECK(&r, r.power_starts == 0); /* ENABLE never raised onto a live module. */
    CHECK(&r, row(&r, "CHECK", BC_MODULE_OFF, "BOOTED stuck high; check BOOTED wiring"));
    CHECK(&r, row(&r, "SKIP", BC_BOOT, "depends on an earlier check"));
    /* Progress is visible while it waits. */
    CHECK(&r, row(&r, "WAIT", BC_MODULE_OFF, "waiting for BOOTED to fall (15 s)"));
    CHECK(&r, !has(&r, "Next steps"));
}

/* ---- Boot ---- */

static void test_power_start_fails(void)
{
    static const bc_status_t want[] = {P, F, S, S, S, S, S, S, S, S};
    static rig_t r;
    rig_init(&r, TOPIC);
    r.start_fails = true;
    run_cycle(&r);
    expect(&r, want);
    expect_verdict(&r, BC_CYCLE_FAIL);
    CHECK(&r, row(&r, "CHECK", BC_BOOT, "serial I/O error"));
}

static void test_boot_never_then_late_boot(void)
{
    static const bc_status_t first[] = {P, F, S, S, S, S, S, S, S, F};
    static const bc_status_t held[] = {F, S, S, S, S, S, S, S, S, S};
    static const bc_status_t good[] = {P, P, P, P, P, P, P, P, P, P};
    static rig_t r;
    rig_init(&r, TOPIC);
    r.boot_delay = -1;
    run_cycle(&r);
    expect(&r, first);
    expect_verdict(&r, BC_CYCLE_FAIL);
    CHECK(&r, row(&r, "CHECK", BC_BOOT, "BOOTED never rose; check supply and ENABLE"));
    CHECK(&r, row(&r, "CHECK", BC_SHUTDOWN, "BOOTED low, ENABLE held; power-cycle board"));
    CHECK(&r, ts_bear_state(&r.bear) == TS_FAULT_HOLD);

    /* The driver keeps ENABLE high for a late boot: the next cycle says so. */
    run_cycle(&r);
    expect(&r, held);
    CHECK(&r, has(&r, "Cycle 2  CHECK"));
    CHECK(&r, row(&r, "CHECK", BC_MODULE_OFF, "driver still holding the module"));
    CHECK(&r, has(&r, "(0/2 cycles passed)"));

    /* The module finally boots; the driver powers it down and recovers. */
    r.m.booted = true;
    idle(&r, TS_BOOT_TIMEOUT_MS);
    CHECK(&r, ts_bear_state(&r.bear) == TS_FAULT);
    r.boot_delay = 500;
    run_cycle(&r);
    expect(&r, good);
    CHECK(&r, has(&r, "Cycle 3  PASS"));
    CHECK(&r, has(&r, "(1/3 cycles passed)"));
}

static void test_slow_boot_shows_progress(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, S, S, P};
    static rig_t r;
    rig_init(&r, 0);
    r.boot_delay = 20000;
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "WAIT", BC_BOOT, "waiting for BOOTED to rise (15 s)"));
    CHECK(&r, row(&r, "PASS", BC_BOOT, "BOOTED high after 20000 ms"));
}

/* ---- Handshake ---- */

static void handshake_fails(const char *prefix, const char *reply, bc_check_t failed,
                            const char *detail)
{
    static rig_t r;
    bc_status_t want[BC_CHECK_COUNT];
    unsigned i;
    rig_init(&r, TOPIC);
    setup_reply(&r, prefix, reply);
    run_cycle(&r);
    for (i = 0; i < BC_CHECK_COUNT; ++i)
        want[i] = i < (unsigned)failed ? P : i == (unsigned)failed ? F : S;
    want[BC_SHUTDOWN] = P; /* The driver forces the module off. */
    expect(&r, want);
    expect_verdict(&r, BC_CYCLE_FAIL);
    CHECK(&r, row(&r, "CHECK", failed, detail));
    CHECK(&r, row(&r, "PASS", BC_SHUTDOWN, "module off after fault"));
    CHECK(&r, ts_bear_state(&r.bear) == TS_FAULT && !r.m.booted);
}

static void test_handshake_failures(void)
{
    handshake_fails("GET apiVersion", "", BC_LINK, "no reply; check TX/RX crossover and baud");
    handshake_fails("GET apiVersion", "garbage\r", BC_LINK, "no reply");
    handshake_fails("GET apiVersion", "500 apiVersion {}\r", BC_LINK, "unexpected reply");
    handshake_fails("GET apiVersion", "200 apiVersion {}\r", BC_LINK, "unexpected reply");
    handshake_fails("GET simConfig", "", BC_SIM, "timed out");
    handshake_fails("PUT simConfig", "400 simConfig {}\r", BC_SIM, "unexpected reply");
    handshake_fails("GET operationalState", "", BC_ACTIVE, "timed out");
    handshake_fails("GET operationalState", "200 operationalState {\"state\":\"test\"}\r",
                    BC_ACTIVE, "unexpected reply");
    handshake_fails("GET operationalState", "200 operationalState {}\r",
                    BC_ACTIVE, "unexpected reply");
    handshake_fails("PUT operationalState {\"state\":\"active\"",
                    "409 operationalState {}\r", BC_ACTIVE, "unexpected reply");
}

/* ---- Information requests ---- */

static void test_imei_failures(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, F, P, S, S, P};
    static const char *const replies[] = {"", "400 hwInfo {}\r", "200 hwInfo {}\r"};
    static const char *const details[] = {"timed out", "unexpected reply", "unexpected reply"};
    static rig_t r;
    unsigned i;
    for (i = 0; i < 3u; ++i) {
        rig_init(&r, 0);
        setup_reply(&r, "GET hwInfo", replies[i]);
        run_cycle(&r);
        expect(&r, want);
        expect_verdict(&r, BC_CYCLE_FAIL);
        CHECK(&r, row(&r, "CHECK", BC_IMEI, details[i]));
        CHECK(&r, !has(&r, "Next steps")); /* No IMEI to activate. */
    }
}

static void test_signal_bars(void)
{
    static rig_t r;
    char reply[64], text[32];
    int bars;
    for (bars = 0; bars <= 5; ++bars) {
        bc_status_t want[] = {P, P, P, P, P, P, P, P, P, P};
        want[BC_SIGNAL] = bars ? P : W;
        rig_init(&r, TOPIC);
        snprintf(reply, sizeof reply, "200 constellationState {\"signal_bars\":%d}\r", bars);
        setup_reply(&r, "GET constellationState", reply);
        run_cycle(&r);
        expect(&r, want);
        expect_verdict(&r, bars ? BC_CYCLE_PASS : BC_CYCLE_WARN);
        snprintf(text, sizeof text, "%d/5 bars", bars);
        CHECK(&r, row(&r, bars ? "PASS" : "WARN", BC_SIGNAL, text));
        if (!bars) {
            CHECK(&r, row(&r, "WARN", BC_SIGNAL, "antenna needs open sky"));
            CHECK(&r, has(&r, "9 pass  1 warn  0 check  0 skip"));
            CHECK(&r, has(&r, "(1/1 cycles passed)")); /* WARN still counts. */
        }
    }
}

static void test_signal_failures(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, F, P, P, P};
    static const char *const replies[] = {
        "", "400 constellationState {}\r", "200 constellationState {}\r",
        "200 constellationState {\"signal_bars\":9}\r"
    };
    static rig_t r;
    unsigned i;
    for (i = 0; i < 4u; ++i) {
        rig_init(&r, TOPIC);
        setup_reply(&r, "GET constellationState", replies[i]);
        run_cycle(&r);
        expect(&r, want);
        expect_verdict(&r, BC_CYCLE_FAIL);
        CHECK(&r, row(&r, "CHECK", BC_SIGNAL, i ? "unexpected reply" : "timed out"));
    }
}

/* ---- Sending ---- */

static void test_send_rejected(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, F, S, P};
    static rig_t r;
    rig_init(&r, TOPIC);
    setup_reply(&r, "PUT messageOriginate ",
                "200 messageOriginate {\"topic_id\":1234,\"message_id\":9,"
                "\"message_response\":\"message_rejected\"}\r");
    run_cycle(&r);
    expect(&r, want);
    expect_verdict(&r, BC_CYCLE_FAIL);
    CHECK(&r, row(&r, "CHECK", BC_SEND, "rejected by modem"));
    CHECK(&r, !bear_checkout_sent(&r.ck));
}

static void test_send_unanswered(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, F, S, P};
    static rig_t r;
    rig_init(&r, TOPIC);
    setup_reply(&r, "PUT messageOriginate ", "");
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "CHECK", BC_SEND, "timed out"));
    CHECK(&r, row(&r, "PASS", BC_SHUTDOWN, "module off after fault"));
    CHECK(&r, sim_saw(&r.m, TS_EVENT_MO_FAILED));
}

static void test_delivery_failures(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, P, F, P};
    static const char *const replies[] = {
        "299 messageOriginateStatus {\"topic_id\":1234,\"message_id\":9,"
        "\"final_mo_status\":\"mo_ack_not_received\"}\r",
        "",
        "299 messageOriginateSegment {\"topic_id\":1234,\"message_id\":9,"
        "\"segment_length\":99,\"segment_start\":0}\r",
        "400 messageOriginateSegment {}\r"
    };
    static const char *const details[] = {
        "network did not confirm", "not confirmed in time; outcome unknown",
        "unexpected reply", "network did not confirm"
    };
    static rig_t r;
    unsigned i;
    for (i = 0; i < 4u; ++i) {
        uint32_t took;
        rig_init(&r, TOPIC);
        setup_reply(&r, "PUT messageOriginateSegment", replies[i]);
        took = run_cycle(&r);
        expect(&r, want);
        expect_verdict(&r, BC_CYCLE_FAIL);
        CHECK(&r, row(&r, "CHECK", BC_DELIVERY, details[i]));
        /* Accepted counts as sent: the sequence number has been used. */
        CHECK(&r, bear_checkout_sent(&r.ck));
        if (i == 1u) {
            CHECK(&r, took >= TS_MO_TIMEOUT_MS);
            CHECK(&r, row(&r, "WAIT", BC_DELIVERY, "waiting for satellite confirmation"));
        }
    }
}

static void test_bad_segment_request(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, P, F, P};
    static rig_t r;
    rig_init(&r, TOPIC);
    /* The modem asks for bytes past the end of the message. */
    setup_reply(&r, "PUT messageOriginate ",
                "200 messageOriginate {\"topic_id\":1234,\"message_id\":9,"
                "\"message_response\":\"message_accepted\"}\r"
                "299 messageOriginateSegment {\"topic_id\":1234,\"message_id\":9,"
                "\"segment_length\":50,\"segment_start\":0}\r");
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "CHECK", BC_DELIVERY, "unexpected reply"));
}

static void test_payload_choices(void)
{
    static const bc_status_t skipped[] = {P, P, P, P, P, P, P, S, S, P};
    static uint8_t big[TS_MAX_PAYLOAD + 1u];
    static rig_t r;

    rig_init(&r, TOPIC);
    run_cycle_with(&r, NULL, 0);
    expect(&r, skipped);
    CHECK(&r, row(&r, "SKIP", BC_SEND, "no payload to send"));
    CHECK(&r, !has(&r, "Next steps")); /* Topic is set: nothing to activate. */

    rig_init(&r, TOPIC);
    run_cycle_with(&r, big, sizeof big); /* Too large: never sent. */
    expect(&r, skipped);

    rig_init(&r, 0);
    run_cycle_with(&r, payload, sizeof payload); /* Topic unset wins. */
    expect(&r, skipped);
    CHECK(&r, row(&r, "SKIP", BC_SEND, "TS_TOPIC_ID not set"));

    rig_init(&r, TOPIC);
    run_cycle_with(&r, big, TS_MAX_PAYLOAD); /* Largest allowed. */
    CHECK(&r, bear_checkout_status(&r.ck, BC_DELIVERY) == BC_PASS);
}

/* ---- Received messages during a cycle ---- */

#if TS_ENABLE_MT
static void queue_mt(rig_t *r, unsigned id, bool corrupt)
{
    uint8_t message[5] = {1, 2, 3, 0, 0};
    uint16_t crc = ts_crc16(message, 3);
    char line[256], b64[16];
    message[3] = (uint8_t)(crc >> 8);
    message[4] = (uint8_t)(crc & 0xffu);
    if (corrupt) message[0] ^= 0xffu;
    assert(ts_base64_encode(message, 5, b64, sizeof b64));
    snprintf(line, sizeof line, "299 messageTerminate {\"topic_id\":77,\"message_id\":%u,"
             "\"message_length_max\":5}\r", id);
    sim_queue(&r->m, line);
    snprintf(line, sizeof line, "299 messageTerminateSegment {\"topic_id\":77,"
             "\"message_id\":%u,\"segment_length\":5,\"segment_start\":0,"
             "\"data\":\"%s\"}\r", id, b64);
    sim_queue(&r->m, line);
    snprintf(line, sizeof line, "299 messageTerminateStatus {\"topic_id\":77,"
             "\"message_id\":%u,\"final_mt_status\":\"complete\"}\r", id);
    sim_queue(&r->m, line);
}

static void mt_when_ready(rig_t *r)
{
    if (r->hook_state == 0 && ts_bear_is_ready(&r->bear)) {
        queue_mt(r, 513, false);
        queue_mt(r, 514, true);
        r->hook_state = 1;
    }
}

static void test_mt_during_cycle(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, P, P, P};
    static rig_t r;
    rig_init(&r, TOPIC);
    r.hook = mt_when_ready;
    run_cycle(&r);
    expect(&r, want);
    expect_verdict(&r, BC_CYCLE_PASS);
    CHECK(&r, has(&r, "PASS   Received message     3 bytes on topic 77 (discarded)"));
    CHECK(&r, has(&r, "WARN   Received message     incomplete or corrupt; dropped"));
    CHECK(&r, !r.bear.mt_available);
}
#endif

/* ---- Power down ---- */

static void test_shutdown_failures(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, P, P, F};
    static rig_t r;

    rig_init(&r, TOPIC);
    setup_reply(&r, SHUTDOWN_CMD, "400 operationalState {}\r");
    run_cycle(&r);
    expect(&r, want);
    expect_verdict(&r, BC_CYCLE_FAIL);
    CHECK(&r, row(&r, "CHECK", BC_SHUTDOWN, "modem refused shutdown"));
    CHECK(&r, !r.m.booted); /* The driver still powered it down. */

    rig_init(&r, TOPIC);
    setup_reply(&r, SHUTDOWN_CMD, "");
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "CHECK", BC_SHUTDOWN, "timed out"));
}

static void test_booted_stuck_after_shutdown(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, P, P, F};
    static const bc_status_t next[] = {F, S, S, S, S, S, S, S, S, S};
    static rig_t r;
    rig_init(&r, TOPIC);
    r.fall_delay = -1;
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "CHECK", BC_SHUTDOWN, "BOOTED still high; module did not stop"));
    CHECK(&r, row(&r, "WAIT", BC_SHUTDOWN, "waiting for BOOTED to fall"));
    run_cycle(&r);
    expect(&r, next);
    CHECK(&r, row(&r, "CHECK", BC_MODULE_OFF, "BOOTED stuck high"));
    CHECK(&r, r.power_starts == 1);
}

/* ---- Faults in the middle of a cycle ---- */

static void drop_booted_at_signal(rig_t *r)
{
    if (r->ck.phase == 4 /* PH_SIGNAL */ && r->hook_state == 0) {
        r->m.booted = false;
        r->hook_state = 1;
    }
}

static void test_booted_drops_mid_cycle(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, F, S, S, F};
    static rig_t r;
    rig_init(&r, TOPIC);
    r.hook = drop_booted_at_signal;
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "CHECK", BC_SIGNAL, "serial I/O error or BOOTED dropped"));
    CHECK(&r, row(&r, "CHECK", BC_SHUTDOWN, "BOOTED low, ENABLE held"));
    CHECK(&r, ts_bear_state(&r.bear) == TS_FAULT_HOLD);
}

static void serial_breaks_while_sending(rig_t *r)
{
    if (ts_bear_state(&r->bear) == TS_MO_ACTIVE) r->m.read_error = 1;
}

static void test_serial_error_mid_send(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, P, F, P};
    static rig_t r;
    rig_init(&r, TOPIC);
    r.hook = serial_breaks_while_sending;
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "CHECK", BC_DELIVERY, "serial I/O error"));
    CHECK(&r, row(&r, "PASS", BC_SHUTDOWN, "module off after fault"));
}

static void test_serial_write_error_during_boot(void)
{
    static const bc_status_t want[] = {P, P, F, S, S, S, S, S, S, P};
    static rig_t r;
    rig_init(&r, TOPIC);
    r.m.write_error = 1;
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "CHECK", BC_LINK, "serial I/O error"));
}

static void test_cycle_not_owned(void)
{
    static const bc_status_t want[] = {F, S, S, S, S, S, S, S, S, S};
    static rig_t r;
    rig_init(&r, TOPIC);
    /* The application started the modem itself. */
    assert(ts_bear_start(&r.bear) == TS_OK);
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "CHECK", BC_MODULE_OFF, "driver still holding the module"));
}

/* ---- Other code sharing the driver ---- */

/* The application asks for the signal itself whenever the checkout is about
 * to use the modem. Each request makes the driver busy for a moment; the
 * checkout must wait its turn, not report a failure. */
static void app_queries(rig_t *r)
{
    uint8_t phase = r->ck.phase;
    if (phase != r->hook_state && ts_bear_is_ready(&r->bear) && !r->bear.query &&
        (phase == 4 /* PH_SIGNAL */ || phase == 5 /* PH_SEND */ ||
         phase == 6 /* PH_SHUTDOWN */)) {
        CHECK(r, (phase == 4 ? ts_bear_request_imei(&r->bear)
                             : ts_bear_request_signal(&r->bear)) == TS_OK);
        r->hook_state = phase;
    }
}

static void test_shared_driver_waits_its_turn(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, P, P, P};
    static rig_t r;
    rig_init(&r, TOPIC);
    r.hook = app_queries;
    run_cycle(&r);
    expect(&r, want);
    expect_verdict(&r, BC_CYCLE_PASS);
    CHECK(&r, r.hook_state == 6);
    CHECK(&r, occurrences(r.m.sent_log, "GET constellationState") == 3u);
    CHECK(&r, occurrences(r.m.sent_log, "GET hwInfo") == 2u); /* Ours (or the driver's) + the app's. */
    CHECK(&r, occurrences(r.m.sent_log, "PUT messageOriginate ") == 1u);
}

static void app_shuts_down(rig_t *r)
{
    if (r->ck.phase == 4 /* PH_SIGNAL */ && r->hook_state == 0) {
        CHECK(r, ts_bear_shutdown(&r->bear) == TS_OK);
        r->hook_state = 1;
    }
}

/* As above, but the application also polls the driver itself, so the
 * checkout next looks after the module is already off. */
static void app_shuts_down_and_polls(rig_t *r)
{
    int n;
    if (r->ck.phase == r->hook_state + 4u && r->hook_state < 3) {
        if (r->hook_state == 2 && r->bear.query) return; /* Shutdown would be BUSY. */
        CHECK(r, ts_bear_shutdown(&r->bear) == TS_OK);
        for (n = 0; n < 100 && ts_bear_state(&r->bear) != TS_OFF; ++n) {
            r->m.now++;
            ts_bear_poll(&r->bear);
        }
        CHECK(r, ts_bear_state(&r->bear) == TS_OFF);
        r->hook_state = 3;
    }
}

static void test_module_stopped_by_other_code(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, F, S, S, P};
    static rig_t r;
    rig_init(&r, TOPIC);
    r.hook = app_shuts_down;
    run_cycle(&r);
    expect(&r, want);
    expect_verdict(&r, BC_CYCLE_FAIL);
    CHECK(&r, row(&r, "CHECK", BC_SIGNAL, "driver busy"));
    CHECK(&r, row(&r, "PASS", BC_SHUTDOWN, "BOOTED low after"));
    CHECK(&r, ts_bear_state(&r.bear) == TS_OFF && !r.m.booted);

    /* BOOTED falls at once: the driver goes from READY to OFF in one poll. */
    rig_init(&r, TOPIC);
    r.fall_delay = 0;
    r.hook = app_shuts_down;
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "CHECK", BC_SIGNAL, "driver busy"));
    CHECK(&r, row(&r, "PASS", BC_SHUTDOWN, "BOOTED low after"));

    /* Already off when the checkout next looks: in the signal, send and
     * shutdown phases. The checkout must finish, not wait forever. */
    {
        static const bc_status_t at_send[] = {P, P, P, P, P, P, P, F, S, P};
        static const bc_status_t at_shutdown[] = {P, P, P, P, P, P, P, P, P, P};
        const bc_status_t *wants[] = {want, at_send, at_shutdown};
        unsigned phase;
        for (phase = 0; phase < 3u; ++phase) {
            rig_init(&r, TOPIC);
            r.fall_delay = 0;
            r.hook_state = (int)phase;
            r.hook = app_shuts_down_and_polls;
            run_cycle(&r);
            expect(&r, wants[phase]);
            CHECK(&r, r.hook_state == 3);
        }
    }
}

/* Message events for a message the checkout did not send are not its own. */
static void test_foreign_message_events(void)
{
    static rig_t r;
    unsigned i;
    rig_init(&r, TOPIC);
    assert(bear_checkout_begin(&r.ck, 0, payload, sizeof payload));
    while (r.ck.phase != 3 /* PH_IMEI */) { tick(&r); (void)bear_checkout_poll(&r.ck, r.m.now); }
    CHECK(&r, bear_checkout_verdict(&r.ck) == BC_CYCLE_FAIL); /* Not finished. */
    bear_checkout_on_event(&r.ck, TS_EVENT_MO_ACCEPTED, 7);
    bear_checkout_on_event(&r.ck, TS_EVENT_MO_FAILED, TS_ERR_REJECTED);
    bear_checkout_on_event(&r.ck, TS_EVENT_MO_DELIVERED, 7);
    bear_checkout_on_event(&r.ck, TS_EVENT_STOPPED, 0);
    for (i = 0; i < 300u; ++i) bear_checkout_on_event(&r.ck, TS_EVENT_MT_DROPPED, 0);
    CHECK(&r, bear_checkout_status(&r.ck, BC_SEND) == BC_WAIT);
    CHECK(&r, bear_checkout_status(&r.ck, BC_DELIVERY) == BC_WAIT);
    CHECK(&r, bear_checkout_status(&r.ck, BC_SHUTDOWN) == BC_WAIT);
    CHECK(&r, !bear_checkout_sent(&r.ck));
    while (!bear_checkout_poll(&r.ck, r.m.now)) tick(&r);
    CHECK(&r, occurrences(r.out, "WARN   Received message     incomplete or corrupt") == 1u);
    CHECK(&r, row(&r, "PASS", BC_SEND, "accepted as message"));

    /* Every other check passed but power-down is still running: not a pass yet. */
    rig_init(&r, TOPIC);
    assert(bear_checkout_begin(&r.ck, 0, payload, sizeof payload));
    while (r.ck.phase != 6 /* PH_SHUTDOWN */) { tick(&r); (void)bear_checkout_poll(&r.ck, r.m.now); }
    CHECK(&r, bear_checkout_status(&r.ck, BC_DELIVERY) == BC_PASS);
    CHECK(&r, bear_checkout_status(&r.ck, BC_SHUTDOWN) == BC_RUN);
    CHECK(&r, bear_checkout_verdict(&r.ck) == BC_CYCLE_FAIL);
    while (!bear_checkout_poll(&r.ck, r.m.now)) tick(&r);
    CHECK(&r, bear_checkout_verdict(&r.ck) == BC_CYCLE_PASS);
}

/* ---- Power-down outcomes after a fault ---- */

static void test_fault_power_down_outcomes(void)
{
    static const bc_status_t want[] = {P, P, F, S, S, S, S, S, S, F};
    static const bc_status_t off[] = {P, P, F, S, S, S, S, S, S, P};
    static rig_t r;

    /* No reply and BOOTED never falls: the module is still running. */
    rig_init(&r, TOPIC);
    r.fall_delay = -1;
    setup_reply(&r, "GET apiVersion", "");
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "CHECK", BC_SHUTDOWN, "BOOTED still high; module did not stop"));
    CHECK(&r, ts_bear_state(&r.bear) == TS_FAULT && r.m.booted);

    /* BOOTED falls the instant ENABLE drops: off within the same poll. */
    rig_init(&r, TOPIC);
    r.fall_delay = 0;
    setup_reply(&r, "GET apiVersion", "");
    run_cycle(&r);
    expect(&r, off);
    CHECK(&r, row(&r, "PASS", BC_SHUTDOWN, "module off after fault"));
}

/* The module browns out after being asked to shut down, before replying. */
static void brownout_at_shutdown(rig_t *r)
{
    if (r->hook_state == 0 && strstr(r->m.sent_log, SHUTDOWN_CMD)) {
        r->m.booted = false;
        r->hook_state = 1;
    }
}

static void test_brownout_during_shutdown(void)
{
    static const bc_status_t want[] = {P, P, P, P, P, P, P, P, P, F};
    static rig_t r;
    rig_init(&r, TOPIC);
    setup_reply(&r, SHUTDOWN_CMD, "");
    r.hook = brownout_at_shutdown;
    run_cycle(&r);
    expect(&r, want);
    CHECK(&r, row(&r, "CHECK", BC_SHUTDOWN, "BOOTED low, ENABLE held; power-cycle board"));
    CHECK(&r, ts_bear_state(&r.bear) == TS_FAULT_HOLD);
}

static void test_reason_texts(void)
{
    static const struct { int32_t reason; const char *text; } table[] = {
        {TS_ERR_TIMEOUT, "timed out"},
        {TS_ERR_IO, "serial I/O error or BOOTED dropped"},
        {TS_ERR_PROTOCOL, "unexpected reply from modem"},
        {TS_ERR_REJECTED, "rejected by modem"},
        {TS_ERR_CAPACITY, "message too large"},
        {TS_ERR_BUSY, "driver busy"},
        {TS_ERR_NOT_READY, "modem not ready"},
        {TS_ERR_ARGUMENT, "invalid argument"},
        {TS_OK, "error"},
        {-999, "error"},
    };
    size_t i;
    for (i = 0; i < sizeof table / sizeof table[0]; ++i)
        assert(!strcmp(bear_checkout_reason(table[i].reason), table[i].text));
}

/* ---- Console behaviour ---- */

static void test_compact_soak_and_tally(void)
{
    static const int passed[] = {0, 1, 2, 2, 3, 4};
    static rig_t r;
    int cycle;
    rig_init(&r, TOPIC);
    for (cycle = 1; cycle <= 5; ++cycle) {
        if (cycle == 3) setup_reply(&r, "GET constellationState", "");
        if (cycle == 4) r.m.rule_count = 0;
        run_cycle(&r);
        idle(&r, 1000);
        if (cycle == 1) {
            CHECK(&r, has(&r, "Next cycle in 600 s (1/1 cycles passed)"));
        } else {
            char text[64];
            /* One line per good cycle; problems add their own rows. */
            CHECK(&r, !has(&r, "BEAR Module 9704 checkout"));
            CHECK(&r, !has(&r, "PASS   Module boot"));
            snprintf(text, sizeof text, "Cycle %d  %s", cycle, cycle == 3 ? "CHECK" : "PASS ");
            CHECK(&r, has(&r, text));
            snprintf(text, sizeof text, "(%d/%d cycles passed)", passed[cycle], cycle);
            CHECK(&r, has(&r, text));
            if (cycle == 3) {
                CHECK(&r, row(&r, "CHECK", BC_SIGNAL, "timed out"));
                CHECK(&r, occurrences(cycle_text(&r), "\r\n") == 2u);
            } else {
                CHECK(&r, occurrences(cycle_text(&r), "\r\n") == 1u);
            }
        }
    }
    CHECK(&r, r.ck.cycles_passed == 4u);
}

static void test_verbose_every_cycle(void)
{
    static rig_t r;
    rig_init(&r, TOPIC);
    r.cfg.verbose_all = true;
    bear_checkout_init(&r.ck, &r.bear, &r.cfg);
    run_cycle(&r);
    run_cycle(&r);
    CHECK(&r, has(&r, "BEAR Module 9704 checkout"));
    CHECK(&r, has(&r, "Next cycle in 600 s (2/2 cycles passed)"));
}

static void test_color(void)
{
    static rig_t r;
    rig_init(&r, 0);
    r.cfg.color = true;
    bear_checkout_init(&r.ck, &r.bear, &r.cfg);
    run_cycle(&r);
    CHECK(&r, has(&r, "\x1b[1;32mPASS\x1b[0m   Module boot"));
    CHECK(&r, has(&r, "\x1b[2mSKIP\x1b[0m   Message queued"));
    CHECK(&r, has(&r, "\x1b[2m[    0."));

    rig_init(&r, TOPIC);
    r.cfg.color = true;
    r.boot_delay = -1;
    bear_checkout_init(&r.ck, &r.bear, &r.cfg);
    run_cycle(&r);
    CHECK(&r, has(&r, "\x1b[1;31mCHECK\x1b[0m  Module boot"));
    CHECK(&r, has(&r, "\x1b[36mWAIT\x1b[0m   Module boot"));

    rig_init(&r, TOPIC);
    r.cfg.color = true;
    setup_reply(&r, "GET constellationState", "200 constellationState {\"signal_bars\":0}\r");
    bear_checkout_init(&r.ck, &r.bear, &r.cfg);
    run_cycle(&r);
    CHECK(&r, has(&r, "\x1b[1;33mWARN\x1b[0m   Satellite signal"));
}

static void test_optional_header_and_long_text(void)
{
    static char board[300];
    static rig_t r;
    rig_init(&r, 0);
    memset(board, 'B', sizeof board - 1u);
    r.cfg.board = board;
    r.cfg.wiring = NULL;
    r.cfg.console = NULL;
    bear_checkout_init(&r.ck, &r.bear, &r.cfg);
    run_cycle(&r); /* rig_console asserts every line fits. */
    CHECK(&r, has(&r, " Board    BBBB"));
    CHECK(&r, !has(&r, " Wiring"));
    CHECK(&r, !has(&r, " Console"));

    rig_init(&r, 0);
    r.cfg.board = NULL;
    bear_checkout_init(&r.ck, &r.bear, &r.cfg);
    run_cycle(&r);
    CHECK(&r, !has(&r, " Board "));
    CHECK(&r, has(&r, "BEAR Module 9704 checkout"));
}

/* The same cycle with a console that refuses writes must print the same rows
 * exactly once each: nothing dropped, nothing repeated. */
static void test_console_back_pressure(void)
{
    static rig_t r;
    static const bc_status_t want[] = {P, P, P, P, P, P, P, P, P, P};
    unsigned every;
    for (every = 2; every <= 5; ++every) {
        rig_init(&r, TOPIC);
        r.refuse_every = every;
        run_cycle(&r);
        expect(&r, want);
        CHECK(&r, r.refused > 0u);
        CHECK(&r, has(&r, "Next cycle in 600 s (1/1 cycles passed)"));
    }

    /* Console stalled for 40 s: the cycle runs on and the report catches up. */
    rig_init(&r, 0);
    r.stall_until = 40000u;
    run_cycle(&r);
    CHECK(&r, r.m.now >= 40000u);
    expect(&r, (const bc_status_t[]){P, P, P, P, P, P, P, S, S, P});
    CHECK(&r, has(&r, "Activate IMEI " SIM_IMEI));
    CHECK(&r, occurrences(r.out, "BEAR Module 9704 checkout") == 1u);
}

/* At most a few lines per poll, so the driver keeps getting polled. */
static void test_render_is_bounded(void)
{
    static rig_t r;
    rig_init(&r, 0);
    r.stall_until = 30000u;
    assert(bear_checkout_begin(&r.ck, 0, NULL, 0));
    while (r.m.now < 30000u) { tick(&r); (void)bear_checkout_poll(&r.ck, r.m.now); }
    r.stall_until = 0;
    {
        unsigned before = r.writes;
        (void)bear_checkout_poll(&r.ck, r.m.now);
        CHECK(&r, r.writes - before == 8u); /* BC_MAX_LINES_PER_POLL */
    }
    while (!bear_checkout_poll(&r.ck, r.m.now)) tick(&r);
}

static void test_api_edges(void)
{
    static rig_t r;
    rig_init(&r, 0);
    assert(!bear_checkout_running(&r.ck));
    assert(!bear_checkout_poll(&r.ck, 0)); /* Nothing started. */
    bear_checkout_on_event(&r.ck, TS_EVENT_SIGNAL, 3); /* Ignored when idle. */
    assert(r.ck.bars == 0);
    assert(!strcmp(bear_checkout_label(BC_CHECK_COUNT), "?"));
    assert(!strcmp(bear_checkout_label(BC_SHUTDOWN), "Power down"));
    assert(bear_checkout_status(&r.ck, BC_CHECK_COUNT) == BC_SKIP);
    assert(r.out_len == 0);
}

/* ---- Transcript for documentation ---- */

static void print_transcript(uint16_t topic)
{
    static rig_t r;
    rig_init(&r, topic);
    r.cfg.board = "MSP430F169, XT2 8 MHz, watchdog 0";
    r.cfg.wiring = "ENABLE P1.0, BOOTED P1.1, UART1 TX P3.6 / RX P3.7";
    r.cfg.console = "USART0 TX P3.4, 115200 8N1; modem 230400 8N1";
    bear_checkout_init(&r.ck, &r.bear, &r.cfg);
    run_cycle(&r);
    idle(&r, 600000u - r.m.now);
    run_cycle(&r);
    fputs(r.out, stdout);
}

int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "transcript")) {
        print_transcript(argc > 2 ? (uint16_t)atoi(argv[2]) : 0u);
        return 0;
    }
    test_api_edges();
    test_activation_cycle();
    test_send_delivered();
    test_already_configured_module();
    test_api_version_selected();
    test_booted_high_then_falls();
    test_booted_stuck_high();
    test_power_start_fails();
    test_boot_never_then_late_boot();
    test_slow_boot_shows_progress();
    test_handshake_failures();
    test_imei_failures();
    test_signal_bars();
    test_signal_failures();
    test_send_rejected();
    test_send_unanswered();
    test_delivery_failures();
    test_bad_segment_request();
    test_payload_choices();
#if TS_ENABLE_MT
    test_mt_during_cycle();
#endif
    test_shutdown_failures();
    test_booted_stuck_after_shutdown();
    test_booted_drops_mid_cycle();
    test_serial_error_mid_send();
    test_serial_write_error_during_boot();
    test_cycle_not_owned();
    test_shared_driver_waits_its_turn();
    test_module_stopped_by_other_code();
    test_foreign_message_events();
    test_fault_power_down_outcomes();
    test_brownout_during_shutdown();
    test_reason_texts();
    test_compact_soak_and_tally();
    test_verbose_every_cycle();
    test_color();
    test_optional_header_and_long_text();
    test_console_back_pressure();
    test_render_is_bounded();
    puts("checkout tests passed");
    return 0;
}
