/* SPDX-License-Identifier: MIT */
#undef NDEBUG /* Tests must check in every build type. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sim_modem.h"
#include "ts_bear9704.h"
#include "ts_internal.h"

/* Failure paths: malformed replies, rejected requests, timeouts, serial
 * errors and bad arguments. The happy path is in test_sdk.c. */

#define TOPIC 1234u
#define SETUP_STEPS 3000

static void begin(ts_bear9704_t *b, sim_modem_t *m)
{
    ts_platform_t p;
    sim_init(m);
    p = sim_platform(m);
    assert(ts_bear_init(b, &p, sim_on_event, m) == TS_OK);
}

static void power_on(ts_bear9704_t *b, sim_modem_t *m)
{
    assert(ts_bear_start(b) == TS_OK);
    m->booted = true;
    sim_run(b, m, SETUP_STEPS);
}

static void ready(ts_bear9704_t *b, sim_modem_t *m)
{
    begin(b, m);
    power_on(b, m);
    assert(ts_bear_is_ready(b));
    m->event_count = 0;
}

/* A macro so a failed assertion reports the calling test's line. */
#define assert_faulted(b, m, reason) \
    do { \
        assert(ts_bear_state(b) == TS_FAULT); \
        assert(sim_saw((m), TS_EVENT_ERROR) && (m)->last_detail == (int32_t)(reason)); \
    } while (0)

/* ---- Start-up ---- */

static void test_setup_chooses_api_version(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    begin(&b, &m);
    sim_override(&m, "GET apiVersion",
        "200 apiVersion {\"supported_versions\":[{\"major\":1,\"minor\":2,\"patch\":3}]}\r");
    power_on(&b, &m);
    assert(ts_bear_is_ready(&b));
    assert(strstr(m.sent_log, "\"major\":1,\"minor\":2,\"patch\":3"));
}

static void test_setup_skips_steps_already_done(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    begin(&b, &m);
    sim_override(&m, "GET simConfig", "200 simConfig {\"interface\":\"internal\"}\r");
    sim_override(&m, "GET operationalState", "200 operationalState {\"state\":\"active\"}\r");
    power_on(&b, &m);
    assert(ts_bear_is_ready(&b));
    assert(!strstr(m.sent_log, "PUT simConfig") && !strstr(m.sent_log, "PUT operationalState"));
}

static void test_setup_failures(void)
{
    static const struct {
        const char *prefix;
        const char *reply;
    } cases[] = {
        {"GET apiVersion", "200 apiVersion {}\r"},
        {"GET apiVersion", "400 apiVersion {}\r"},
        {"GET simConfig", "400 simConfig {}\r"},
        {"GET operationalState", "200 operationalState {\"state\":\"test\"}\r"},
        {"GET operationalState", "200 operationalState {}\r"},
        {"PUT operationalState", "400 operationalState {}\r"},
    };
    size_t i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        ts_bear9704_t b;
        sim_modem_t m;
        begin(&b, &m);
        sim_override(&m, cases[i].prefix, cases[i].reply);
        power_on(&b, &m);
        assert(!ts_bear_is_ready(&b));
        assert_faulted(&b, &m, TS_ERR_PROTOCOL);
    }
}

static void test_setup_ignores_unrelated_replies(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ts_jspr_frame_t f;
    char line[] = "200 operationalState {}";
    begin(&b, &m);
    sim_override(&m, "GET apiVersion",
        "200 somethingElse {}\r"
        "200 apiVersion {\"active_version\":{\"major\":1,\"minor\":6,\"patch\":0}}\r");
    power_on(&b, &m);
    assert(ts_bear_is_ready(&b));

    assert(ts_jspr_parse(line, &f));
    b.state = TS_BOOTING; /* Not a handshake state. */
    assert(ts_setup_on_frame(&b, &f));
}

static void test_boot_timeout_and_unresponsive_modem(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    begin(&b, &m);
    sim_override(&m, "GET apiVersion", ""); /* Booted, but never answers. */
    power_on(&b, &m);
    sim_run(&b, &m, (int)TS_CMD_TIMEOUT_MS);
    assert_faulted(&b, &m, TS_ERR_TIMEOUT);
}

static void test_start_fails_when_command_cannot_be_queued(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    begin(&b, &m);
    assert(ts_bear_start(&b) == TS_OK);
    b.tx_len = 3; /* A previous command is still being written. */
    m.booted = true;
    sim_run(&b, &m, 5);
    assert_faulted(&b, &m, TS_ERR_IO);
}

/* ---- Serial and line handling ---- */

static void test_serial_errors_fault_the_driver(void)
{
    ts_bear9704_t b;
    sim_modem_t m;

    ready(&b, &m);
    m.read_error = 1;
    sim_run(&b, &m, 5);
    assert_faulted(&b, &m, TS_ERR_IO);

    ready(&b, &m);
    assert(ts_bear_request_signal(&b) == TS_OK);
    m.write_error = 1;
    sim_run(&b, &m, 5);
    assert_faulted(&b, &m, TS_ERR_IO);

    ready(&b, &m);
    assert(ts_bear_request_signal(&b) == TS_OK);
    m.write_error = 2; /* Claims to have written more than it was given. */
    sim_run(&b, &m, 5);
    assert_faulted(&b, &m, TS_ERR_IO);
}

static void test_bad_lines_are_reported_not_fatal(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    char big[401];

    ready(&b, &m);
    sim_queue(&m, "this is not a modem line\r");
    sim_run(&b, &m, 40);
    assert(sim_saw(&m, TS_EVENT_ERROR) && m.last_detail == TS_ERR_PROTOCOL);
    assert(ts_bear_is_ready(&b));

    /* A line longer than the buffer is skipped up to its end. */
    m.event_count = 0;
    memset(big, 'a', sizeof big - 1u);
    big[sizeof big - 1u] = '\0';
    sim_queue(&m, big);
    sim_queue(&m, "\r");
    sim_run(&b, &m, 40);
    assert(sim_saw(&m, TS_EVENT_ERROR) && ts_bear_is_ready(&b));

    /* The driver still works afterwards. */
    assert(ts_bear_request_signal(&b) == TS_OK);
    sim_run(&b, &m, 50);
    assert(ts_bear_signal_bars(&b) == 4);
}

/* ---- Information requests ---- */

static void test_query_failures(void)
{
    ts_bear9704_t b;
    sim_modem_t m;

    begin(&b, &m);
    sim_override(&m, "GET hwInfo", "200 hwInfo {}\r");
    power_on(&b, &m);
    assert(ts_bear_is_ready(&b) && ts_bear_imei(&b) == NULL);
    assert(sim_saw(&m, TS_EVENT_ERROR));

    ready(&b, &m);
    sim_override(&m, "GET constellationState", "200 constellationState {\"signal_bars\":9}\r");
    assert(ts_bear_request_signal(&b) == TS_OK);
    sim_run(&b, &m, 50);
    assert(sim_saw(&m, TS_EVENT_ERROR) && ts_bear_signal_bars(&b) == -1);

    ready(&b, &m);
    sim_override(&m, "GET constellationState", "");
    assert(ts_bear_request_signal(&b) == TS_OK);
    assert(ts_bear_request_imei(&b) == TS_ERR_NOT_READY); /* One at a time. */
    sim_run(&b, &m, (int)TS_CMD_TIMEOUT_MS + 50);
    assert(sim_saw(&m, TS_EVENT_ERROR) && m.last_detail == TS_ERR_TIMEOUT);
    assert(ts_bear_is_ready(&b)); /* A lost query is not fatal. */

    ready(&b, &m);
    b.tx_len = 1;
    b.tx_pos = 0;
    assert(ts_bear_request_signal(&b) == TS_ERR_BUSY);
}

/* ---- Arguments and states ---- */

static void test_argument_checks(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ts_platform_t good, p;
    const uint8_t data[] = {1};

    begin(&b, &m);
    good = sim_platform(&m);
    assert(ts_bear_init(NULL, &good, NULL, NULL) == TS_ERR_ARGUMENT);
    assert(ts_bear_init(&b, NULL, NULL, NULL) == TS_ERR_ARGUMENT);
    p = good; p.serial_read = NULL;  assert(ts_bear_init(&b, &p, NULL, NULL) == TS_ERR_ARGUMENT);
    p = good; p.serial_write = NULL; assert(ts_bear_init(&b, &p, NULL, NULL) == TS_ERR_ARGUMENT);
    p = good; p.millis = NULL;       assert(ts_bear_init(&b, &p, NULL, NULL) == TS_ERR_ARGUMENT);
    p = good; p.power_start = NULL;  assert(ts_bear_init(&b, &p, NULL, NULL) == TS_ERR_ARGUMENT);
    p = good; p.power_stop = NULL;   assert(ts_bear_init(&b, &p, NULL, NULL) == TS_ERR_ARGUMENT);
    p = good; p.uart_enable = NULL;  assert(ts_bear_init(&b, &p, NULL, NULL) == TS_ERR_ARGUMENT);
    p = good; p.boot_ready = NULL;   assert(ts_bear_init(&b, &p, NULL, NULL) == TS_ERR_ARGUMENT);
    assert(ts_bear_init(&b, &good, NULL, NULL) == TS_OK); /* No callback is fine. */

    assert(ts_bear_init(&b, &good, NULL, NULL) == TS_OK);
    assert(ts_bear_start(NULL) == TS_ERR_ARGUMENT);
    assert(ts_bear_shutdown(NULL) == TS_ERR_ARGUMENT);
    assert(ts_bear_shutdown(&b) == TS_OK); /* Already off. */
    assert(ts_bear_send(NULL, TOPIC, data, 1) == TS_ERR_ARGUMENT);
    assert(ts_bear_request_signal(NULL) == TS_ERR_NOT_READY);
    assert(ts_bear_send(&b, TOPIC, data, 1) == TS_ERR_NOT_READY);
    assert(!ts_bear_is_ready(NULL) && ts_bear_state(NULL) == TS_OFF);
    assert(ts_bear_imei(NULL) == NULL && ts_bear_signal_bars(NULL) == -1);
    ts_bear_poll(NULL);
    ts_bear_poll(&b); /* Off: nothing to do. */
}

static bool power_start_fails(void *user)
{
    (void)user;
    return false;
}

static void test_start_failure(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ts_platform_t p;
    sim_init(&m);
    p = sim_platform(&m);
    p.power_start = power_start_fails;
    assert(ts_bear_init(&b, &p, sim_on_event, &m) == TS_OK);
    assert(ts_bear_start(&b) == TS_ERR_IO);
    assert(ts_bear_state(&b) == TS_OFF);
}

static void test_send_and_shutdown_guards(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    uint8_t big[TS_MAX_PAYLOAD + 1u] = {0};
    const uint8_t data[] = {1, 2};

    ready(&b, &m);
    assert(ts_bear_start(&b) == TS_ERR_BUSY); /* Already running. */
    assert(ts_bear_send(&b, TOPIC, big, sizeof big) == TS_ERR_CAPACITY);
    b.tx_len = 1;
    b.tx_pos = 0;
    assert(ts_bear_send(&b, TOPIC, data, sizeof data) == TS_ERR_IO);
    assert(ts_bear_shutdown(&b) == TS_ERR_IO);

    ready(&b, &m);
    assert(ts_bear_request_signal(&b) == TS_OK);
    assert(ts_bear_send(&b, TOPIC, data, sizeof data) == TS_ERR_NOT_READY);
    assert(ts_bear_shutdown(&b) == TS_ERR_BUSY);

    ready(&b, &m);
    assert(ts_bear_send(&b, TOPIC, data, sizeof data) == TS_OK);
    assert(ts_bear_shutdown(&b) == TS_ERR_BUSY); /* Not while sending. */

    ready(&b, &m);
    b.request_reference = TS_MAX_REQUEST_REFERENCE;
    assert(ts_bear_send(&b, TOPIC, data, sizeof data) == TS_OK);
    assert(b.request_reference == 1u); /* Wraps instead of overflowing. */
}

/* ---- Shutdown ---- */

static void test_shutdown_failures(void)
{
    ts_bear9704_t b;
    sim_modem_t m;

    ready(&b, &m);
    sim_override(&m, "PUT operationalState", "400 operationalState {}\r");
    assert(ts_bear_shutdown(&b) == TS_OK);
    sim_run(&b, &m, 3000);
    assert_faulted(&b, &m, TS_ERR_REJECTED);

    ready(&b, &m);
    sim_override(&m, "PUT operationalState", "");
    assert(ts_bear_shutdown(&b) == TS_OK);
    sim_run(&b, &m, (int)TS_CMD_TIMEOUT_MS + 3000);
    assert_faulted(&b, &m, TS_ERR_TIMEOUT);

    /* The module never reports that it has shut down. */
    ready(&b, &m);
    m.hold_booted = true;
    assert(ts_bear_shutdown(&b) == TS_OK);
    sim_run(&b, &m, (int)TS_BOOT_TIMEOUT_MS + 100);
    assert_faulted(&b, &m, TS_ERR_TIMEOUT);
    assert(!sim_saw(&m, TS_EVENT_STOPPED));
}

/* ---- Sending ---- */

static void send_one(ts_bear9704_t *b, sim_modem_t *m, int steps)
{
    const uint8_t data[] = {1, 2, 3};
    assert(ts_bear_send(b, TOPIC, data, sizeof data) == TS_OK);
    sim_run(b, m, steps);
}

static void test_send_rejections(void)
{
    ts_bear9704_t b;
    sim_modem_t m;

    ready(&b, &m);
    sim_override(&m, "PUT messageOriginate ", "400 messageOriginate {}\r");
    send_one(&b, &m, 50);
    assert(sim_saw(&m, TS_EVENT_MO_FAILED) && m.last_detail == TS_ERR_REJECTED);
    assert(ts_bear_is_ready(&b));

    ready(&b, &m);
    sim_override(&m, "PUT messageOriginate ",
        "200 messageOriginate {\"topic_id\":1234,\"message_id\":9,"
        "\"message_response\":\"message_rejected\"}\r");
    send_one(&b, &m, 50);
    assert(sim_saw(&m, TS_EVENT_MO_FAILED) && ts_bear_is_ready(&b));

    ready(&b, &m);
    sim_override(&m, "PUT messageOriginate ", "");
    send_one(&b, &m, (int)TS_CMD_TIMEOUT_MS + 50);
    assert(sim_saw(&m, TS_EVENT_MO_FAILED));
    assert_faulted(&b, &m, TS_ERR_TIMEOUT);
}

static void test_send_protocol_errors(void)
{
    ts_bear9704_t b;
    sim_modem_t m;

    /* The modem asks for bytes beyond the end of the message. */
    ready(&b, &m);
    sim_override(&m, "PUT messageOriginate ",
        "200 messageOriginate {\"topic_id\":1234,\"message_id\":9,"
        "\"message_response\":\"message_accepted\"}\r"
        "299 messageOriginateSegment {\"topic_id\":1234,\"message_id\":9,"
        "\"segment_length\":999,\"segment_start\":0}\r");
    send_one(&b, &m, 100);
    assert(sim_saw(&m, TS_EVENT_MO_FAILED) && m.last_detail == TS_ERR_PROTOCOL);
    assert(ts_bear_is_ready(&b));

    /* The modem rejects the bytes we sent. */
    ready(&b, &m);
    sim_override(&m, "PUT messageOriginateSegment", "400 messageOriginateSegment {}\r");
    send_one(&b, &m, 100);
    assert(sim_saw(&m, TS_EVENT_MO_FAILED) && m.last_detail == TS_ERR_REJECTED);

    /* The network reports failure. */
    ready(&b, &m);
    sim_override(&m, "PUT messageOriginateSegment",
        "299 messageOriginateStatus {\"topic_id\":1234,\"message_id\":9,"
        "\"final_mo_status\":\"mo_timeout\"}\r");
    send_one(&b, &m, 100);
    assert(sim_saw(&m, TS_EVENT_MO_FAILED) && !sim_saw(&m, TS_EVENT_MO_DELIVERED));
    assert(ts_bear_is_ready(&b));
}

static void test_send_ignores_other_messages_then_times_out(void)
{
    ts_bear9704_t b;
    sim_modem_t m;

    ready(&b, &m);
    sim_override(&m, "PUT messageOriginateSegment",
        "299 messageOriginateStatus {\"topic_id\":1234,\"message_id\":77,"
        "\"final_mo_status\":\"mo_ack_received\"}\r"
        "299 somethingElse {}\r");
    send_one(&b, &m, 100);
    assert(!sim_saw(&m, TS_EVENT_MO_DELIVERED));
    assert(ts_bear_state(&b) == TS_MO_ACTIVE);
    sim_run(&b, &m, (int)TS_MO_TIMEOUT_MS);
    assert(sim_saw(&m, TS_EVENT_MO_FAILED) && m.last_detail == TS_ERR_TIMEOUT);
    assert(ts_bear_is_ready(&b)); /* The modem is fine; the message is not. */
}

static void test_segment_request_while_tx_busy(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ts_jspr_frame_t f;
    char line[] = "299 messageOriginateSegment {\"topic_id\":1234,\"message_id\":9,"
                  "\"segment_length\":3,\"segment_start\":0}";

    ready(&b, &m);
    assert(ts_jspr_parse(line, &f));
    ts_mo_on_frame(&b, &f); /* Nothing pending: ignored. */
    assert(ts_bear_is_ready(&b));
    b.mo_pending = true;
    b.state = TS_MO_ACTIVE;
    b.topic = TOPIC;
    b.message_id = 9;
    b.mo_length = 5;
    b.tx_len = 1;
    b.tx_pos = 0;
    ts_mo_on_frame(&b, &f);
    sim_run(&b, &m, 5); /* Finish powering down. */
    assert_faulted(&b, &m, TS_ERR_PROTOCOL);
    assert(sim_saw(&m, TS_EVENT_MO_FAILED));
}

/* ---- Receiving ---- */

#if TS_ENABLE_MT
static void queue_announce(sim_modem_t *m, unsigned topic, unsigned id, unsigned size)
{
    char line[160];
    (void)snprintf(line, sizeof line,
        "299 messageTerminate {\"topic_id\":%u,\"message_id\":%u,\"message_length_max\":%u}\r",
        topic, id, size);
    sim_queue(m, line);
}

static void queue_segment(sim_modem_t *m, unsigned start, const uint8_t *bytes, size_t n)
{
    char b64[32], line[220];
    assert(ts_base64_encode(bytes, n, b64, sizeof b64));
    (void)snprintf(line, sizeof line,
        "299 messageTerminateSegment {\"topic_id\":77,\"message_id\":5,"
        "\"segment_length\":%u,\"segment_start\":%u,\"data\":\"%s\"}\r",
        (unsigned)n, start, b64);
    sim_queue(m, line);
}

static void test_receive_failures(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    const uint8_t x[2] = {1, 2}, y[2] = {1, 3};

    /* A second announcement while one is in progress. */
    ready(&b, &m);
    queue_announce(&m, 77, 5, 6);
    queue_announce(&m, 77, 6, 6);
    sim_run(&b, &m, 100);
    assert(sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_BUSY);

    /* Invalid announcements: reserved topic, too large, too small. */
    ready(&b, &m);
    queue_announce(&m, 10, 5, 6);
    sim_run(&b, &m, 100);
    assert(m.last_detail == TS_ERR_CAPACITY && sim_saw(&m, TS_EVENT_MT_DROPPED));
    m.event_count = 0;
    queue_announce(&m, 77, 5, TS_MAX_PAYLOAD + 3u);
    queue_announce(&m, 77, 5, 1);
    sim_run(&b, &m, 200);
    assert(sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_CAPACITY);

    /* A segment that is not valid base64, or the wrong length. */
    ready(&b, &m);
    queue_announce(&m, 77, 5, 6);
    sim_queue(&m, "299 messageTerminateSegment {\"topic_id\":77,\"message_id\":5,"
                  "\"segment_length\":2,\"segment_start\":0,\"data\":\"!!!!\"}\r");
    sim_run(&b, &m, 200);
    assert(sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_PROTOCOL);

    /* The same bytes sent twice must match. */
    ready(&b, &m);
    queue_announce(&m, 77, 5, 6);
    queue_segment(&m, 0, x, 2);
    queue_segment(&m, 0, x, 2);
    sim_run(&b, &m, 400);
    assert(!sim_saw(&m, TS_EVENT_MT_DROPPED));
    queue_segment(&m, 0, y, 2);
    sim_run(&b, &m, 200);
    assert(sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_PROTOCOL);

    /* A final status that is not "complete", or arrives with gaps. */
    ready(&b, &m);
    queue_announce(&m, 77, 5, 6);
    sim_queue(&m, "299 messageTerminateStatus {\"topic_id\":77,\"message_id\":5,"
                  "\"final_mt_status\":\"complete\"}\r");
    sim_run(&b, &m, 200);
    assert(sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_PROTOCOL);

    /* The modem goes quiet part way through. */
    ready(&b, &m);
    queue_announce(&m, 77, 5, 6);
    sim_run(&b, &m, (int)TS_MT_TIMEOUT_MS + 100);
    assert(sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_TIMEOUT);

    /* take_mt checks its arguments and state. */
    {
        uint8_t out[4];
        size_t length;
        uint16_t topic;
        ready(&b, &m);
        assert(ts_bear_take_mt(&b, out, sizeof out, &length, &topic) == TS_ERR_NOT_READY);
        assert(ts_bear_take_mt(NULL, out, sizeof out, &length, &topic) == TS_ERR_ARGUMENT);
        assert(ts_bear_take_mt(&b, NULL, sizeof out, &length, &topic) == TS_ERR_ARGUMENT);
        assert(ts_bear_take_mt(&b, out, sizeof out, NULL, &topic) == TS_ERR_ARGUMENT);
        assert(ts_bear_take_mt(&b, out, sizeof out, &length, NULL) == TS_ERR_ARGUMENT);
    }
}
#else
static void test_receive_disabled(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    uint8_t out[4];
    size_t length;
    uint16_t topic;
    ready(&b, &m);
    assert(ts_bear_take_mt(&b, out, sizeof out, &length, &topic) == TS_ERR_NOT_READY);
}
#endif

/* ---- Helpers ---- */

static void test_json_parsing_edges(void)
{
    uint32_t v, a, b, c;
    char text[8];
    const char *span;
    size_t span_len;

    assert(ts_json_u32("{\"a\":\"x\\\"y\\\\\",\"n\":5}", "n", &v) && v == 5u);
    assert(!ts_json_u32("{\"a\":\"x\\qy\",\"n\":5}", "n", &v));    /* Bad escape. */
    assert(!ts_json_u32("{\"a\":\"x\\", "n", &v));                 /* Ends in escape. */
    assert(!ts_json_u32("{\"a\":\"abc", "n", &v));                 /* Unterminated. */
    assert(!ts_json_u32("{\"a\":\"a\tb\",\"n\":5}", "n", &v));     /* Control byte. */
    assert(!ts_json_u32("{\"a\":1 \"n\":5}", "n", &v));            /* Missing comma. */
    assert(!ts_json_u32("{\"a\":1", "n", &v));                     /* Truncated. */
    assert(!ts_json_u32("{", "n", &v));
    assert(!ts_json_u32("{\"a\":1,", "n", &v));                    /* Ends after a comma. */
    assert(!ts_json_u32("{\"n\":99999999999}", "n", &v));          /* Too large. */
    assert(!ts_json_u32("{\"n\":-1}", "n", &v));
    assert(!ts_json_u32("{\"n\":\"5\"}", "n", &v));
    assert(!ts_json_u32(NULL, "n", &v));

    assert(ts_json_string("{\"s\":\"abc\"}", "s", text, sizeof text));
    assert(!ts_json_string("{\"s\":\"a\\\"b\"}", "s", text, sizeof text)); /* Escapes refused. */
    assert(!ts_json_string("{\"s\":\"abcdefghij\"}", "s", text, sizeof text)); /* Too long. */
    assert(!ts_json_string("{\"s\":5}", "s", text, sizeof text));
    assert(!ts_json_string("{\"s\":\"a\tb\"}", "s", text, sizeof text));
    assert(ts_json_string_span("{\"s\":\"abc\"}", "s", &span, &span_len) && span_len == 3u);
    assert(!ts_json_string_span("{\"s\":5}", "s", &span, &span_len));

    assert(ts_json_first_version("{\"supported_versions\":[{\"major\":1,\"minor\":2,\"patch\":3}]}",
                                 &a, &b, &c) && a == 1u && b == 2u && c == 3u);
    assert(!ts_json_first_version("{}", &a, &b, &c));
    assert(!ts_json_first_version("{\"supported_versions\":5}", &a, &b, &c));
    assert(!ts_json_first_version("{\"supported_versions\":[]}", &a, &b, &c));
    assert(!ts_json_version("{\"active_version\":{\"major\":1}}", "active_version", &a, &b, &c));
}

static void test_jspr_frame_edges(void)
{
    ts_jspr_frame_t f;
    char a[] = "";
    char b[] = "20 x {}";
    char c[] = "abc x {}";
    char d[] = "200";
    char e[] = "200 target";
    char g[] = "200 target {}";
    char h[] = "9999 target {}";
    assert(!ts_jspr_parse(a, &f));
    assert(!ts_jspr_parse(b, &f));
    assert(!ts_jspr_parse(c, &f));
    assert(!ts_jspr_parse(d, &f));
    assert(!ts_jspr_parse(e, &f));
    assert(ts_jspr_parse(g, &f) && f.code == 200u);
    assert(!ts_jspr_parse(h, &f));
    assert(!ts_jspr_parse(NULL, &f));
}

static void test_command_builder_overflow(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ts_cmd_t cmd;
    char filler[TS_LINE_CAP + 1u];

    begin(&b, &m);
    memset(filler, 'x', sizeof filler - 1u);
    filler[sizeof filler - 1u] = '\0';

    assert(ts_cmd_begin(&b, &cmd));
    ts_cmd_str(&cmd, filler); /* Too long. */
    ts_cmd_str(&cmd, "x");    /* Stays overflowed. */
    assert(!ts_cmd_end(&b, &cmd));

    assert(ts_cmd_begin(&b, &cmd));
    filler[TS_LINE_CAP - 3u] = '\0'; /* Leaves room for two characters. */
    ts_cmd_str(&cmd, filler);
    ts_cmd_u32(&cmd, 12345u); /* Does not fit. */
    ts_cmd_u32(&cmd, 1u);     /* Stays overflowed. */
    ts_cmd_base64(&cmd, (const uint8_t *)"a", 1);
    assert(!ts_cmd_end(&b, &cmd));
}

static void test_base64_decoding(void)
{
    uint8_t out[8];
    size_t n;
    assert(ts_base64_decode("AAECAwQ=", 8, out, sizeof out, &n) && n == 5u && out[4] == 4u);
    assert(ts_base64_decode("AAEC", 4, out, sizeof out, &n) && n == 3u);
    assert(ts_base64_decode("+/8=", 4, out, sizeof out, &n) && out[0] == 0xfbu);
    assert(!ts_base64_decode("AA*C", 4, out, sizeof out, &n));   /* Not base64. */
    assert(!ts_base64_decode("AAE", 3, out, sizeof out, &n));    /* Bad length. */
    assert(!ts_base64_decode("AA=C", 4, out, sizeof out, &n));   /* Padding in the middle. */
    assert(!ts_base64_decode("AAECAwQ=", 8, out, 2, &n));        /* Too small. */
    assert(!ts_base64_decode("AB==", 4, out, sizeof out, &n));   /* Stray bits. */
    assert(!ts_base64_decode(NULL, 4, out, sizeof out, &n));
    assert(ts_base64_encode((const uint8_t *)"a", 1, (char[4]){0}, 4) == 0u); /* No room for NUL. */
}

static void test_names_cover_every_value(void)
{
    static const ts_state_t states[] = {
        TS_OFF, TS_BOOTING, TS_API_QUERY, TS_API_SELECT, TS_SIM_QUERY, TS_SIM_SET,
        TS_MODE_QUERY, TS_MODE_SET, TS_READY, TS_MO_START, TS_MO_ACTIVE,
        TS_SHUTDOWN_WAIT, TS_POWERING_DOWN, TS_FAULT};
    static const ts_event_t events[] = {
        TS_EVENT_READY, TS_EVENT_MO_ACCEPTED, TS_EVENT_MO_DELIVERED, TS_EVENT_MO_FAILED,
        TS_EVENT_MT_RECEIVED, TS_EVENT_MT_DROPPED, TS_EVENT_SIGNAL, TS_EVENT_IMEI,
        TS_EVENT_STOPPED, TS_EVENT_ERROR};
    static const ts_result_t results[] = {
        TS_OK, TS_ERR_ARGUMENT, TS_ERR_BUSY, TS_ERR_CAPACITY, TS_ERR_IO, TS_ERR_PROTOCOL,
        TS_ERR_TIMEOUT, TS_ERR_REJECTED, TS_ERR_NOT_READY};
    size_t i;
    for (i = 0; i < sizeof states / sizeof states[0]; ++i)
        assert(strncmp(ts_state_name(states[i]), "TS_", 3) == 0 &&
               strcmp(ts_state_name(states[i]), "TS_STATE_UNKNOWN") != 0);
    for (i = 0; i < sizeof events / sizeof events[0]; ++i)
        assert(strcmp(ts_event_name(events[i]), "TS_EVENT_UNKNOWN") != 0);
    for (i = 0; i < sizeof results / sizeof results[0]; ++i)
        assert(strcmp(ts_result_name(results[i]), "TS_ERR_UNKNOWN") != 0);
    assert(!strcmp(ts_event_name((ts_event_t)99), "TS_EVENT_UNKNOWN"));
    assert(!strcmp(ts_result_name((ts_result_t)99), "TS_ERR_UNKNOWN"));
}

int main(void)
{
    test_setup_chooses_api_version();
    test_setup_skips_steps_already_done();
    test_setup_failures();
    test_setup_ignores_unrelated_replies();
    test_boot_timeout_and_unresponsive_modem();
    test_start_fails_when_command_cannot_be_queued();
    test_serial_errors_fault_the_driver();
    test_bad_lines_are_reported_not_fatal();
    test_query_failures();
    test_argument_checks();
    test_start_failure();
    test_send_and_shutdown_guards();
    test_shutdown_failures();
    test_send_rejections();
    test_send_protocol_errors();
    test_send_ignores_other_messages_then_times_out();
    test_segment_request_while_tx_busy();
#if TS_ENABLE_MT
    test_receive_failures();
#else
    test_receive_disabled();
#endif
    test_json_parsing_edges();
    test_jspr_frame_edges();
    test_command_builder_overflow();
    test_base64_decoding();
    test_names_cover_every_value();
    puts("PASS: Telemetry Station SDK failure-path tests");
    return 0;
}
