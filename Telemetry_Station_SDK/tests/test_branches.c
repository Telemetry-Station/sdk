/* SPDX-License-Identifier: MIT */
#undef NDEBUG /* Tests must check in every build type. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "sim_modem.h"
#include "ts_activation.h"
#include "ts_bear9704.h"
#include "ts_gnss.h"
#include "ts_internal.h"

/* Branch-level checks: every decision in src/ is driven both ways and the
 * observable outcome asserted. Grouped by module. Happy paths are in
 * test_sdk.c and broader failure scenarios in test_faults.c. */

#define TOPIC 1234u
#define SETUP_STEPS 3000

static const uint8_t PAYLOAD[5] = {1, 2, 3, 4, 5};

static void begin(ts_bear9704_t *b, sim_modem_t *m)
{
    ts_platform_t p;
    sim_init(m);
    p = sim_platform(m);
    assert(ts_bear_init(b, &p, sim_on_event, m) == TS_OK);
}

/* With TS_QUERY_IMEI_ON_READY off the application asks for the IMEI itself;
 * do so here so every configuration reaches the same READY state. */
static void app_requests_imei(ts_bear9704_t *b, sim_modem_t *m)
{
#if TS_QUERY_IMEI_ON_READY
    (void)b;
    (void)m;
#else
    assert(ts_bear_imei(b) == NULL && !strstr(m->sent_log, "GET hwInfo"));
    assert(ts_bear_request_imei(b) == TS_OK);
    sim_run(b, m, 50);
#endif
}

static void ready(ts_bear9704_t *b, sim_modem_t *m)
{
    begin(b, m);
    assert(ts_bear_start(b) == TS_OK);
    m->booted = true;
    sim_run(b, m, SETUP_STEPS);
    assert(ts_bear_is_ready(b));
    app_requests_imei(b, m);
    assert(ts_bear_imei(b) && !strcmp(ts_bear_imei(b), SIM_IMEI));
    m->event_count = 0;
    m->sent_log[0] = '\0';
}

/* Delivers one line from the modem and lets the driver process it. */
static void feed(ts_bear9704_t *b, sim_modem_t *m, const char *line)
{
    sim_queue(m, line);
    sim_queue(m, "\r");
    sim_run(b, m, 10);
}

static int count(const sim_modem_t *m, ts_event_t event)
{
    int i, n = 0;
    for (i = 0; i < m->event_count; ++i)
        if (m->events[i] == event) ++n;
    return n;
}

static bool parse(const char *text, ts_jspr_frame_t *f)
{
    static char line[TS_LINE_CAP];
    assert(strlen(text) < sizeof line);
    strcpy(line, text);
    return ts_jspr_parse(line, f);
}

/* ---- ts_activation.c ---- */

static void test_activation_text(void)
{
    char out[400], expected[400];
    const unsigned payloads[] = {0u, 120u, UINT_MAX};
    size_t full, cap, i;
    const char *imei = SIM_IMEI;

    /* Null output, zero capacity, invalid IMEI. */
    assert(ts_activation_text(imei, 0, NULL, sizeof out) == 0);
    assert(ts_activation_text(imei, 0, out, 0) == 0);
    assert(ts_activation_text("12345", 0, out, sizeof out) == 0);
    assert(ts_activation_text(NULL, 0, out, sizeof out) == 0);

    for (i = 0; i < sizeof payloads / sizeof payloads[0]; ++i) {
        int n;
        if (payloads[i]) {
            n = snprintf(expected, sizeof expected,
                         "Modem IMEI: %s\r\n"
                         "1. Size your plan: " TS_CALCULATOR_URL "?payloadBytes=%u\r\n"
                         "2. Activate: " TS_ACTIVATE_URL "\r\n", imei, payloads[i]);
        } else {
            n = snprintf(expected, sizeof expected,
                         "Modem IMEI: %s\r\n"
                         "1. Get payload code and bytes per message: " TS_OPTIMIZER_URL "\r\n"
                         "2. Size your plan: " TS_CALCULATOR_URL "\r\n"
                         "3. Activate: " TS_ACTIVATE_URL "\r\n", imei);
        }
        assert(n > 0 && (size_t)n < sizeof expected);
        full = (size_t)n;
        assert(ts_activation_text(imei, payloads[i], out, sizeof out) == full);
        assert(strcmp(out, expected) == 0);
        memset(out, 'x', sizeof out);
        assert(ts_activation_text(imei, payloads[i], out, 0) == 0 && out[0] == 'x');
        for (cap = 1; cap <= full; ++cap) {
            memset(out, 'x', sizeof out);
            assert(ts_activation_text(imei, payloads[i], out, cap) == 0 && out[0] == '\0');
            assert(out[cap] == 'x');
        }
        memset(out, 'x', sizeof out);
        assert(ts_activation_text(imei, payloads[i], out, full + 1u) == full);
        assert(strcmp(out, expected) == 0 && out[full + 1u] == 'x');
    }
}

/* ---- ts_base64.c ---- */

static void test_base64_encode_arguments(void)
{
    char out[16];
    const uint8_t abc[3] = {'a', 'b', 'c'};

    assert(ts_base64_encode(NULL, 3, out, sizeof out) == 0);      /* null input with length */
    out[0] = 'x';
    assert(ts_base64_encode(NULL, 0, out, sizeof out) == 0 && out[0] == '\0'); /* empty */
    assert(ts_base64_encode(abc, 3, NULL, sizeof out) == 0);      /* null output */
    assert(ts_base64_encode(NULL, 0, NULL, sizeof out) == 0);     /* empty input, null output */
    out[0] = 'x';
    assert(ts_base64_encode(abc, 0, out, 1) == 0 && out[0] == '\0'); /* empty input, room for NUL only */
    assert(ts_base64_encode(abc, 3, out, 0) == 0);                /* zero capacity */
    out[0] = 'x';
    assert(ts_base64_encode(abc, 0, out, 0) == 0 && out[0] == 'x'); /* empty, zero capacity: no NUL written */
    assert(ts_base64_encode(abc, 3, out, 3) == 0);                /* too short for one group */
    assert(ts_base64_encode(abc, 3, out, 4) == 0);                /* no room for the NUL */
    assert(ts_base64_encode(abc, 3, out, 5) == 4 && !strcmp(out, "YWJj"));
}

static void test_base64_decode_permutations(void)
{
    uint8_t out[8];
    size_t n = 99;

    /* Null pointers and a length that is not a multiple of four. */
    assert(!ts_base64_decode("QQ==", 4, out, sizeof out, NULL));
    assert(!ts_base64_decode(NULL, 4, out, sizeof out, &n) && n == 0);
    assert(!ts_base64_decode("QQ==", 4, NULL, sizeof out, &n));
    assert(!ts_base64_decode("QQ=", 3, out, sizeof out, &n));       /* truncated */

    /* An invalid character in each position of a group. */
    assert(!ts_base64_decode("{QQQ", 4, out, sizeof out, &n));      /* above 'z' */
    assert(!ts_base64_decode("Q!QQ", 4, out, sizeof out, &n));      /* below '0' */
    assert(!ts_base64_decode("QQ:Q", 4, out, sizeof out, &n));      /* between '9' and 'A' */
    assert(!ts_base64_decode("QQQ~", 4, out, sizeof out, &n));
    assert(ts_base64_decode("az09", 4, out, sizeof out, &n) && n == 3); /* range edges */
    assert(ts_base64_decode("+/+/", 4, out, sizeof out, &n) && n == 3);

    /* Misplaced padding. */
    assert(!ts_base64_decode("QQ=A", 4, out, sizeof out, &n));      /* data after '=' */
    assert(!ts_base64_decode("QQ==QUJD", 8, out, sizeof out, &n));  /* "==" not last */
    assert(!ts_base64_decode("QUE=QUJD", 8, out, sizeof out, &n));  /* "=" not last */

    /* Non-canonical bits under the padding. */
    assert(!ts_base64_decode("QR==", 4, out, sizeof out, &n));
    assert(!ts_base64_decode("QUF=", 4, out, sizeof out, &n));

    /* Output overflow by one at each byte of a group. */
    assert(!ts_base64_decode("QQ==", 4, out, 0, &n));
    assert(!ts_base64_decode("QUI=", 4, out, 1, &n));
    assert(!ts_base64_decode("QUJD", 4, out, 2, &n));
    assert(ts_base64_decode("QUJD", 4, out, 3, &n) && n == 3 && !memcmp(out, "ABC", 3));

    /* One- and two-byte final groups. */
    assert(ts_base64_decode("QQ==", 4, out, 1, &n) && n == 1 && out[0] == 'A');
    assert(ts_base64_decode("QUI=", 4, out, 2, &n) && n == 2 && !memcmp(out, "AB", 2));
}

/* ---- ts_crc.c ---- */

static void test_crc_null_input(void)
{
    const uint8_t x[1] = {0x01};
    assert(ts_crc16(NULL, 4) == 0);   /* null data with a length */
    assert(ts_crc16(NULL, 0) == 0);   /* empty input */
    assert(ts_crc16(x, 1) == 0x1021u);
}

/* ---- ts_jspr.c: JSON accessors ---- */

static void test_json_member_lookup(void)
{
    uint32_t v = 0;
    char s[8];

    /* Null JSON, null key, not an object. */
    assert(!ts_json_u32(NULL, "a", &v));
    assert(!ts_json_u32("{\"a\":1}", NULL, &v));
    assert(!ts_json_u32("[1]", "a", &v));

    /* Whitespace of each kind around tokens. */
    assert(ts_json_string("\t{\t\"a\"\t:\t\"x\"\t}", "a", s, sizeof s) && !strcmp(s, "x"));
    assert(ts_json_u32("\n{\n\"a\" :\n7\n}", "a", &v) && v == 7);
    assert(ts_json_u32("{ \"a\":1\n,\"b\":2}", "b", &v) && v == 2); /* scalar ends at '\n' */

    /* Malformed members. */
    assert(!ts_json_u32("{a:1}", "a", &v));                 /* unquoted key */
    assert(!ts_json_u32("{\"a\" 1}", "a", &v));             /* missing colon */
    assert(!ts_json_u32("{\"a\":", "b", &v));               /* missing value */
    assert(!ts_json_u32("{\"a\":1]", "b", &v));             /* scalar ends at ']' */
    assert(!ts_json_u32("{\"a\":tru", "b", &v));            /* truncated scalar */
    assert(!ts_json_u32("{\"a\":1 \"b\":2}", "b", &v));     /* missing comma */
    assert(!ts_json_u32("{\"\":1}", "a", &v));              /* empty key is not a match */
    assert(ts_json_u32("{\"\":1,\"a\":3}", "a", &v) && v == 3);
}

static void test_json_nested_values(void)
{
    uint32_t v = 0;
    static const char deep16[] =
        "{\"a\":[[[[[[[[[[[[[[[[1]]]]]]]]]]]]]]]],\"b\":4}";
    static const char deep17[] =
        "{\"a\":[[[[[[[[[[[[[[[[[1]]]]]]]]]]]]]]]]],\"b\":4}";

    assert(ts_json_u32("{\"a\":{\"x\":\"y\",\"z\":[1,{}]},\"b\":4}", "b", &v) && v == 4);
    assert(!ts_json_u32("{\"a\":{\"x\":\"\\q\"},\"b\":4}", "b", &v));   /* bad escape inside */
    assert(!ts_json_u32("{\"a\":{\"x\":\"\x01\"},\"b\":4}", "b", &v));  /* control char inside */
    assert(!ts_json_u32("{\"a\":[1},\"b\":4}", "b", &v));               /* mismatched close */
    assert(!ts_json_u32("{\"a\":{\"x\":1", "b", &v));                   /* unterminated object */
    assert(ts_json_u32(deep16, "b", &v) && v == 4);                     /* nesting at the limit */
    assert(!ts_json_u32(deep17, "b", &v));                              /* nesting over by one */
}

static void test_json_u32_values(void)
{
    uint32_t v = 0;

    assert(!ts_json_u32("{\"a\":1}", "a", NULL));             /* null output */
    assert(!ts_json_u32("{\"a\":\"1\"}", "a", &v));           /* string, not number */
    assert(!ts_json_u32("{\"a\":x}", "a", &v));               /* above '9' */
    assert(!ts_json_u32("{\"a\":-1}", "a", &v));              /* below '0' */
    assert(!ts_json_u32("{\"a\":4294967296}", "a", &v));      /* overflow by one */
    assert(ts_json_u32("{\"a\":4294967295}", "a", &v) && v == UINT32_MAX);
    assert(ts_json_u32("{\"a\":12 }", "a", &v) && v == 12);   /* ends at ' ' */
    assert(ts_json_u32("{\"a\":13\n}", "a", &v) && v == 13);  /* ends at '\n' */
    assert(ts_json_u32("{\"a\":14,\"b\":0}", "a", &v) && v == 14);
    assert(!ts_json_u32("{\"a\":12x}", "a", &v));             /* trailing junk */
    assert(!ts_json_u32("{\"a\":12]", "a", &v));              /* wrong terminator */
}

static void test_json_string_values(void)
{
    char s[4];
    const char *data = NULL;
    size_t len = 0;

    assert(!ts_json_string("{\"a\":\"x\"}", "a", NULL, sizeof s));  /* null output */
    assert(!ts_json_string("{\"a\":\"x\"}", "a", s, 0));             /* zero capacity */
    assert(!ts_json_string("{\"a\":1}", "a", s, sizeof s));          /* wrong type */
    assert(!ts_json_string("{\"a\":\"abcd\"}", "a", s, sizeof s));   /* overflow by one */
    assert(ts_json_string("{\"a\":\"abc\"}", "a", s, sizeof s) && !strcmp(s, "abc"));
    assert(!ts_json_string("{\"a\":\"a\\\"\"}", "a", s, sizeof s));  /* escaped */
    assert(!ts_json_string("{\"a\":\"\x01\"}", "a", s, sizeof s));   /* control char */
    assert(!ts_json_string("{\"a\":\"abc", "a", s, sizeof s));       /* unterminated */

    assert(!ts_json_string_span(NULL, "a", &data, &len));          /* null JSON */
    assert(!ts_json_string_span("{\"a\":\"x\"}", "missing", &data, &len));
    assert(!ts_json_string_span("{\"a\":\"x\"}", "a", NULL, &len));  /* null data */
    assert(!ts_json_string_span("{\"a\":\"x\"}", "a", &data, NULL)); /* null length */
    assert(!ts_json_string_span("{\"a\":1}", "a", &data, &len));     /* wrong type */
    assert(!ts_json_string_span("{\"a\":\"QQ", "a", &data, &len));   /* unterminated */
    assert(!ts_json_string_span("{\"a\":\"Q\\/Q\"}", "a", &data, &len)); /* escaped */
    assert(ts_json_string_span("{\"a\":\"QQ==\"}", "a", &data, &len) &&
           len == 4 && !strncmp(data, "QQ==", 4));
}

static void test_json_versions(void)
{
    uint32_t ma = 0, mi = 0, pa = 0;
    char big[300];

    assert(ts_json_version("{\"v\":{\"major\":1,\"minor\":2,\"patch\":3}}", "v", &ma, &mi, &pa) &&
           ma == 1 && mi == 2 && pa == 3);
    assert(!ts_json_version("{\"v\":1}", "v", &ma, &mi, &pa));                        /* not an object */
    assert(!ts_json_version("{\"v\":{\"major\":1", "v", &ma, &mi, &pa));             /* unterminated */
    assert(!ts_json_version("{\"v\":{\"minor\":1,\"patch\":1}}", "v", &ma, &mi, &pa)); /* no major */
    assert(!ts_json_version("{\"v\":{\"major\":1,\"patch\":1}}", "v", &ma, &mi, &pa)); /* no minor */
    assert(!ts_json_version("{\"v\":{\"major\":1,\"minor\":1}}", "v", &ma, &mi, &pa)); /* no patch */
    assert(!ts_json_version("{\"v\":{\"major\":256,\"minor\":1,\"patch\":1}}", "v", &ma, &mi, &pa));
    assert(!ts_json_version("{\"v\":{\"major\":1,\"minor\":256,\"patch\":1}}", "v", &ma, &mi, &pa));
    assert(!ts_json_version("{\"v\":{\"major\":1,\"minor\":1,\"patch\":256}}", "v", &ma, &mi, &pa));

    /* A version object longer than the 160-byte scratch buffer. */
    (void)snprintf(big, sizeof big, "{\"v\":{\"major\":1,\"minor\":2,\"patch\":3,\"pad\":\"%0200d\"}}", 0);
    assert(!ts_json_version(big, "v", &ma, &mi, &pa));

    assert(!ts_json_first_version("{}", &ma, &mi, &pa));                              /* missing */
    assert(!ts_json_first_version("{\"supported_versions\":{}}", &ma, &mi, &pa));      /* not a list */
    assert(ts_json_first_version("{\"supported_versions\":[ {\"major\":4,\"minor\":5,\"patch\":6}]}",
                                 &ma, &mi, &pa) && ma == 4 && mi == 5 && pa == 6);
}

/* ---- ts_jspr.c: line framing ---- */

static void test_jspr_parse_permutations(void)
{
    ts_jspr_frame_t f;
    char line[] = "200 a {}";
    char long_target[64];

    assert(!ts_jspr_parse(NULL, &f));                  /* null line */
    assert(!ts_jspr_parse(line, NULL));                /* null frame */

    /* XON, space and newline noise before the code is skipped. */
    assert(parse("\x11 \n200 a {}", &f) && f.code == 200 && !strcmp(f.target, "a"));

    assert(!parse("x00 a {}", &f));                    /* first digit */
    assert(!parse("2x0 a {}", &f));                    /* second digit */
    assert(!parse("20x a {}", &f));                    /* third digit */
    assert(!parse("2000 a {}", &f));                   /* no space after the code */
    assert(!parse("199 a {}", &f));                    /* below the range */
    assert(!parse("600 a {}", &f));                    /* above the range */
    assert(parse("599 a {}", &f) && f.code == 599);
    assert(!parse("200 {}", &f));                      /* no target */
    assert(!parse("200  {}", &f));                     /* empty target */
    assert(!parse("200 a-b {}", &f));                  /* non-alphanumeric target */
    assert(!parse("200 a x", &f));                     /* payload not an object */
    assert(!parse("200 a {", &f));                     /* truncated payload */
    assert(!parse("200 a {} x", &f));                  /* trailing junk */
    assert(parse("200 a {} \n", &f) && !strcmp(f.json, "{} \n"));

    /* Target length: 31 fits, 32 is over by one. */
    (void)snprintf(long_target, sizeof long_target, "200 %031d {}", 0);
    assert(parse(long_target, &f) && strlen(f.target) == 31);
    (void)snprintf(long_target, sizeof long_target, "200 %032d {}", 0);
    assert(!parse(long_target, &f));
}

/* ---- ts_setup.c ---- */

static void test_setup_waits_while_command_in_flight(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ts_jspr_frame_t f;
    begin(&b, &m);

    /* A reply that arrives while our previous command is still going out
     * cannot be acted on: the next command has nowhere to go. */
    assert(parse("200 simConfig {\"interface\":\"internal\"}", &f));
    b.state = TS_SIM_SET;
    b.tx_len = 3;
    b.tx_pos = 0;
    assert(!ts_setup_on_frame(&b, &f) && b.state == TS_SIM_SET);

    /* Same when the API version must be chosen. */
    assert(parse("200 apiVersion {\"supported_versions\":[{\"major\":1,\"minor\":2,\"patch\":3}]}", &f));
    b.state = TS_API_QUERY;
    assert(!ts_setup_on_frame(&b, &f) && b.state == TS_API_QUERY);
}

static void test_setup_sim_interface_missing(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    begin(&b, &m);
    sim_override(&m, "GET simConfig", "200 simConfig {}\r"); /* no interface field */
    assert(ts_bear_start(&b) == TS_OK);
    m.booted = true;
    sim_run(&b, &m, SETUP_STEPS);
    assert(ts_bear_is_ready(&b));
    assert(strstr(m.sent_log, "PUT simConfig {\"interface\":\"internal\"}"));
}

/* ---- ts_bear9704.c ---- */

typedef struct {
    sim_modem_t m;
    ts_bear9704_t *b;
    int action;
} app_t;

enum { APP_NONE, APP_SIGNAL_ON_READY, APP_SEND_ON_READY, APP_DROP_BOOTED_ON_ERROR };

static void app_event(void *user, ts_event_t event, int32_t detail)
{
    app_t *a = user;
    sim_on_event(&a->m, event, detail);
    if (event == TS_EVENT_READY && a->action == APP_SIGNAL_ON_READY)
        assert(ts_bear_request_signal(a->b) == TS_OK);
    if (event == TS_EVENT_READY && a->action == APP_SEND_ON_READY)
        assert(ts_bear_send(a->b, TOPIC, PAYLOAD, sizeof PAYLOAD) == TS_OK);
    if (event == TS_EVENT_ERROR && a->action == APP_DROP_BOOTED_ON_ERROR)
        a->m.booted = false;
}

static void app_begin(app_t *a, ts_bear9704_t *b, int action)
{
    ts_platform_t p;
    sim_init(&a->m);
    a->b = b;
    a->action = action;
    p = sim_platform(&a->m);
    assert(ts_bear_init(b, &p, app_event, a) == TS_OK);
}

static void test_no_event_callback(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ts_platform_t p;
    sim_init(&m);
    p = sim_platform(&m);
    assert(ts_bear_init(&b, &p, NULL, NULL) == TS_OK);
    assert(ts_bear_start(&b) == TS_OK);
    m.booted = true;
    sim_run(&b, &m, SETUP_STEPS);
    assert(ts_bear_is_ready(&b));
    app_requests_imei(&b, &m);
    assert(m.event_count == 0);
    assert(!strcmp(ts_bear_imei(&b), SIM_IMEI)); /* IMEI fetched without anyone listening. */
}

static void test_ready_callback_preempts_imei_query(void)
{
    ts_bear9704_t b;
    app_t a;

    /* The application issues its own query from the READY event. */
    app_begin(&a, &b, APP_SIGNAL_ON_READY);
    assert(ts_bear_start(&b) == TS_OK);
    a.m.booted = true;
    sim_run(&b, &a.m, SETUP_STEPS);
    assert(sim_saw(&a.m, TS_EVENT_SIGNAL) && ts_bear_signal_bars(&b) == 4);
    assert(!strstr(a.m.sent_log, "GET hwInfo") && ts_bear_imei(&b) == NULL);

    /* The application starts sending from the READY event. */
    app_begin(&a, &b, APP_SEND_ON_READY);
    assert(ts_bear_start(&b) == TS_OK);
    a.m.booted = true;
    sim_run(&b, &a.m, SETUP_STEPS);
    assert(sim_saw(&a.m, TS_EVENT_MO_DELIVERED));
    assert(!strstr(a.m.sent_log, "GET hwInfo") && ts_bear_imei(&b) == NULL);
}

static void test_fault_hold_stops_line_processing(void)
{
    ts_bear9704_t b;
    app_t a;

    /* BOOTED falls (here: in the error callback for a garbled line) before a
     * protocol fault. The fault isolates the UART and holds ENABLE; lines
     * already buffered in the same poll must not be processed afterwards. */
    app_begin(&a, &b, APP_DROP_BOOTED_ON_ERROR);
    sim_override(&a.m, "GET apiVersion",
        "garbage\r200 apiVersion {}\rjunk\r");
    assert(ts_bear_start(&b) == TS_OK);
    a.m.booted = true;
    sim_run(&b, &a.m, 20);
    assert(ts_bear_state(&b) == TS_FAULT_HOLD);
    assert(count(&a.m, TS_EVENT_ERROR) == 2 && a.m.last_detail == TS_ERR_PROTOCOL);
}

static void test_shutdown_busy_permutations(void)
{
    ts_bear9704_t b;
    sim_modem_t m;

    /* An information request is outstanding. */
    ready(&b, &m);
    sim_override(&m, "GET constellationState", "");
    assert(ts_bear_request_signal(&b) == TS_OK);
    sim_run(&b, &m, 20);
    assert(ts_bear_shutdown(&b) == TS_ERR_BUSY && ts_bear_state(&b) == TS_READY);

#if TS_ENABLE_MT
    /* A message is being received. */
    ready(&b, &m);
    feed(&b, &m, "299 messageTerminate {\"topic_id\":77,\"message_id\":5,\"message_length_max\":6}");
    assert(b.mt_pending);
    assert(ts_bear_shutdown(&b) == TS_ERR_BUSY && ts_bear_state(&b) == TS_READY);

    /* A received message has not been taken yet. */
    b.mt_pending = false;
    b.mt_available = true;
    assert(ts_bear_shutdown(&b) == TS_ERR_BUSY && ts_bear_state(&b) == TS_READY);
#endif
}

static void test_send_and_query_arguments(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ready(&b, &m);
    assert(ts_bear_send(&b, TOPIC, NULL, 3) == TS_ERR_ARGUMENT);     /* null data */
    assert(ts_bear_send(&b, TOPIC, PAYLOAD, 0) == TS_ERR_ARGUMENT);  /* zero length */
    assert(ts_bear_request_signal(NULL) == TS_ERR_NOT_READY);        /* null context */
    assert(ts_bear_request_imei(NULL) == TS_ERR_NOT_READY);
    assert(ts_bear_state(&b) == TS_READY && m.sent_log[0] == '\0');
}

static void test_query_ignores_other_replies(void)
{
    ts_bear9704_t b;
    sim_modem_t m;

    /* Waiting for signal; an IMEI reply is not the answer. */
    ready(&b, &m);
    sim_override(&m, "GET constellationState",
        "200 hwInfo {\"imei\":\"111111111111111\"}\r200 constellationState {\"signal_bars\":3}\r");
    assert(ts_bear_request_signal(&b) == TS_OK);
    sim_run(&b, &m, 50);
    assert(ts_bear_signal_bars(&b) == 3 && !strcmp(ts_bear_imei(&b), SIM_IMEI));
    assert(count(&m, TS_EVENT_SIGNAL) == 1 && !sim_saw(&m, TS_EVENT_IMEI));

    /* Waiting for the IMEI; a signal reply is not the answer. */
    ready(&b, &m);
    sim_override(&m, "GET hwInfo",
        "200 constellationState {\"signal_bars\":2}\r200 hwInfo {\"imei\":\"111111111111111\"}\r");
    assert(ts_bear_request_imei(&b) == TS_OK);
    sim_run(&b, &m, 50);
    assert(!strcmp(ts_bear_imei(&b), "111111111111111") && ts_bear_signal_bars(&b) == -1);
    assert(count(&m, TS_EVENT_IMEI) == 1 && !sim_saw(&m, TS_EVENT_SIGNAL));

    /* No request outstanding: a stray reply changes nothing. */
    ready(&b, &m);
    feed(&b, &m, "200 hwInfo {\"imei\":\"222222222222222\"}");
    assert(m.event_count == 0 && !strcmp(ts_bear_imei(&b), SIM_IMEI));

    /* Unsolicited traffic with no active transfer is ignored in either build. */
    feed(&b, &m, "299 unrelated {}");
    assert(m.event_count == 0 && !b.mo_pending && !b.mt_pending);
}

static void test_shutdown_ignores_other_replies(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ready(&b, &m);
    sim_override(&m, "PUT operationalState",
        "200 hwInfo {}\r200 operationalState {\"state\":\"inactive\"}\r");
    assert(ts_bear_shutdown(&b) == TS_OK);
    sim_run(&b, &m, 100);
    assert(ts_bear_state(&b) == TS_OFF && sim_saw(&m, TS_EVENT_STOPPED));
    assert(!sim_saw(&m, TS_EVENT_ERROR));
}

static void test_newline_handling(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    /* CRLF endings: a leading '\n' is skipped, one inside a line is kept. */
    ready(&b, &m);
    sim_override(&m, "GET constellationState",
        "\n200 constellationState {\n\"signal_bars\":5}\r\n");
    assert(ts_bear_request_signal(&b) == TS_OK);
    sim_run(&b, &m, 50);
    assert(ts_bear_signal_bars(&b) == 5 && !sim_saw(&m, TS_EVENT_ERROR));
}

/* ---- ts_mo.c ---- */

#define ACCEPTED "200 messageOriginate {\"topic_id\":1234,\"message_id\":9," \
                 "\"message_response\":\"message_accepted\"}\r"

static void mo_start(ts_bear9704_t *b, sim_modem_t *m, const char *accept_reply)
{
    ready(b, m);
    sim_override(m, "PUT messageOriginate ", accept_reply);
    sim_override(m, "PUT messageOriginateSegment", "");
    assert(ts_bear_send(b, TOPIC, PAYLOAD, sizeof PAYLOAD) == TS_OK);
    sim_run(b, m, 50);
}

/* Message 9 on topic 1234 is accepted; 7 bytes (5 + CRC) await requests. */
static void mo_active(ts_bear9704_t *b, sim_modem_t *m)
{
    mo_start(b, m, ACCEPTED);
    assert(ts_bear_state(b) == TS_MO_ACTIVE && sim_saw(m, TS_EVENT_MO_ACCEPTED));
    m->event_count = 0;
    m->sent_log[0] = '\0';
}

static void test_mo_accept_reply_permutations(void)
{
    static const char *const bad[] = {
        /* no message_response */
        "200 messageOriginate {\"topic_id\":1234,\"message_id\":9}\r",
        /* no message_id */
        "200 messageOriginate {\"topic_id\":1234,\"message_response\":\"message_accepted\"}\r",
        /* message_id over 16 bits */
        "200 messageOriginate {\"topic_id\":1234,\"message_id\":65536,"
            "\"message_response\":\"message_accepted\"}\r",
        /* no topic_id */
        "200 messageOriginate {\"message_id\":9,\"message_response\":\"message_accepted\"}\r",
        /* mismatched topic */
        "200 messageOriginate {\"topic_id\":99,\"message_id\":9,"
            "\"message_response\":\"message_accepted\"}\r",
    };
    size_t i;
    for (i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        ts_bear9704_t b;
        sim_modem_t m;
        mo_start(&b, &m, bad[i]);
        assert(ts_bear_state(&b) == TS_READY && !b.mo_pending);
        assert(sim_saw(&m, TS_EVENT_MO_FAILED) && m.last_detail == TS_ERR_REJECTED);
    }
}

static void test_mo_start_ignores_other_lines(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    /* An unsolicited 299 with the same target, and an unrelated reply, are
     * not the acceptance. */
    mo_start(&b, &m, "299 messageOriginate {}\r200 hwInfo {}\r" ACCEPTED);
    assert(ts_bear_state(&b) == TS_MO_ACTIVE);
    assert(count(&m, TS_EVENT_MO_ACCEPTED) == 1 && !sim_saw(&m, TS_EVENT_MO_FAILED));
}

static void test_mo_segment_request_permutations(void)
{
    static const char *const bad[] = {
        "{\"message_id\":9,\"segment_length\":7,\"segment_start\":0}",                /* no topic */
        "{\"topic_id\":1,\"message_id\":9,\"segment_length\":7,\"segment_start\":0}", /* other topic */
        "{\"topic_id\":1234,\"segment_length\":7,\"segment_start\":0}",               /* no id */
        "{\"topic_id\":1234,\"message_id\":8,\"segment_length\":7,\"segment_start\":0}", /* other id */
        "{\"topic_id\":1234,\"message_id\":9,\"segment_length\":7}",                  /* no start */
        "{\"topic_id\":1234,\"message_id\":9,\"segment_start\":0}",                   /* no length */
        "{\"topic_id\":1234,\"message_id\":9,\"segment_length\":0,\"segment_start\":0}", /* empty */
        "{\"topic_id\":1234,\"message_id\":9,\"segment_length\":1,\"segment_start\":8}", /* start past end */
        "{\"topic_id\":1234,\"message_id\":9,\"segment_length\":5,\"segment_start\":3}", /* over by one */
    };
    char line[200];
    size_t i;
    for (i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        ts_bear9704_t b;
        sim_modem_t m;
        mo_active(&b, &m);
        (void)snprintf(line, sizeof line, "299 messageOriginateSegment %s", bad[i]);
        feed(&b, &m, line);
        assert(ts_bear_state(&b) == TS_READY && !b.mo_pending);
        assert(sim_saw(&m, TS_EVENT_MO_FAILED) && m.last_detail == TS_ERR_PROTOCOL);
        assert(!strstr(m.sent_log, "PUT messageOriginateSegment"));
    }

    /* The last byte exactly: answered with that byte. */
    {
        ts_bear9704_t b;
        sim_modem_t m;
        uint32_t deadline;
        mo_active(&b, &m);
        deadline = b.deadline;
        feed(&b, &m, "299 messageOriginateSegment {\"topic_id\":1234,\"message_id\":9,"
                     "\"segment_length\":4,\"segment_start\":3}");
        sim_run(&b, &m, 60); /* the sim writes three bytes per poll */
        assert(ts_bear_state(&b) == TS_MO_ACTIVE && m.event_count == 0);
        assert(strstr(m.sent_log, "\"segment_length\":4,\"segment_start\":3,\"data\":\""));
        assert(b.deadline == deadline);
    }
}

static void test_mo_active_other_lines(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    mo_active(&b, &m);

    /* Ignored: a non-error reply to the segment, an unrelated reply, an
     * unrelated unsolicited line, a status for another message. */
    feed(&b, &m, "200 messageOriginateSegment {}");
    feed(&b, &m, "200 messageOriginateStatus {}");
    feed(&b, &m, "299 somethingElse {}");
    feed(&b, &m, "299 messageOriginateStatus {\"topic_id\":1234,\"message_id\":8,"
                 "\"final_mo_status\":\"mo_ack_received\"}");
    assert(ts_bear_state(&b) == TS_MO_ACTIVE && m.event_count == 0);

    /* A final status without its result field counts as a failure. */
    feed(&b, &m, "299 messageOriginateStatus {\"topic_id\":1234,\"message_id\":9}");
    assert(ts_bear_state(&b) == TS_READY && !b.mo_pending);
    assert(sim_saw(&m, TS_EVENT_MO_FAILED) && m.last_detail == TS_ERR_REJECTED);
}

/* ---- ts_mt.c ---- */

#if TS_ENABLE_MT
#define ANNOUNCE "299 messageTerminate {\"topic_id\":77,\"message_id\":5,\"message_length_max\":6}"

static void mt_pending(ts_bear9704_t *b, sim_modem_t *m)
{
    ready(b, m);
    feed(b, m, ANNOUNCE);
    assert(b->mt_pending && m->event_count == 0);
}

static void feed_segment(ts_bear9704_t *b, sim_modem_t *m, unsigned start,
                         const uint8_t *bytes, size_t n)
{
    char b64[32], line[220];
    assert(ts_base64_encode(bytes, n, b64, sizeof b64));
    (void)snprintf(line, sizeof line,
        "299 messageTerminateSegment {\"topic_id\":77,\"message_id\":5,"
        "\"segment_length\":%u,\"segment_start\":%u,\"data\":\"%s\"}",
        (unsigned)n, start, b64);
    feed(b, m, line);
}

#define COMPLETE "299 messageTerminateStatus {\"topic_id\":77,\"message_id\":5," \
                 "\"final_mt_status\":\"complete\"}"

static void message_with_crc(uint8_t msg[6], bool good_low_byte)
{
    uint16_t crc;
    msg[0] = 1; msg[1] = 2; msg[2] = 3; msg[3] = 4;
    crc = ts_crc16(msg, 4);
    msg[4] = (uint8_t)(crc >> 8);
    msg[5] = (uint8_t)((crc & 0xffu) ^ (good_low_byte ? 0u : 1u));
}

static void test_mt_announce_permutations(void)
{
    static const char *const bad[] = {
        "{\"message_id\":5,\"message_length_max\":6}",                       /* no topic */
        "{\"topic_id\":65536,\"message_id\":5,\"message_length_max\":6}",    /* topic over 16 bits */
        "{\"topic_id\":77,\"message_length_max\":6}",                        /* no id */
        "{\"topic_id\":77,\"message_id\":65536,\"message_length_max\":6}",   /* id over 16 bits */
        "{\"topic_id\":77,\"message_id\":5}",                                /* no length */
    };
    char line[160];
    size_t i;
    for (i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        ts_bear9704_t b;
        sim_modem_t m;
        ready(&b, &m);
        (void)snprintf(line, sizeof line, "299 messageTerminate %s", bad[i]);
        feed(&b, &m, line);
        assert(!b.mt_pending);
        assert(sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_CAPACITY);
    }
}

static void test_mt_announce_while_message_unread(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    uint8_t msg[6], out[8];
    size_t n = 0;
    uint16_t topic = 0;
    message_with_crc(msg, true);
    mt_pending(&b, &m);
    feed_segment(&b, &m, 0, msg, 6);
    feed(&b, &m, COMPLETE);
    assert(sim_saw(&m, TS_EVENT_MT_RECEIVED) && m.last_detail == 4);

    /* A second message before the first is taken is refused. */
    m.event_count = 0;
    feed(&b, &m, ANNOUNCE);
    assert(sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_BUSY);
    assert(ts_bear_take_mt(&b, out, sizeof out, &n, &topic) == TS_OK);
    assert(n == 4 && topic == 77 && !memcmp(out, msg, 4));
}

static void test_mt_segment_permutations(void)
{
    static const char *const bad[] = {
        "\"segment_length\":2,\"data\":\"AQI=\"",                              /* no start */
        "\"segment_start\":0,\"data\":\"AQI=\"",                               /* no length */
        "\"segment_length\":0,\"segment_start\":0,\"data\":\"\"",              /* empty */
        "\"segment_length\":1,\"segment_start\":7,\"data\":\"AQ==\"",          /* start past end */
        "\"segment_length\":3,\"segment_start\":4,\"data\":\"AQID\"",          /* over by one */
        "\"segment_length\":2,\"segment_start\":0",                            /* no data */
        "\"segment_length\":2,\"segment_start\":0,\"data\":\"AQ\\/=\"",        /* escaped data */
        "\"segment_length\":2,\"segment_start\":0,\"data\":\"AQ=\"",           /* truncated base64 */
        "\"segment_length\":2,\"segment_start\":0,\"data\":\"AQ==\"",          /* length mismatch */
    };
    char line[200];
    size_t i;
    for (i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        ts_bear9704_t b;
        sim_modem_t m;
        mt_pending(&b, &m);
        (void)snprintf(line, sizeof line,
            "299 messageTerminateSegment {\"topic_id\":77,\"message_id\":5,%s}", bad[i]);
        feed(&b, &m, line);
        assert(!b.mt_pending);
        assert(sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_PROTOCOL);
    }
}

static void test_mt_ignores_other_messages(void)
{
    ts_bear9704_t b;
    sim_modem_t m;
    ts_jspr_frame_t f;
    mt_pending(&b, &m);

    /* Lines for another transfer or of another kind leave this one alone. */
    feed(&b, &m, "299 messageTerminateStatus {\"message_id\":5,\"final_mt_status\":\"complete\"}");
    feed(&b, &m, "299 messageTerminateStatus {\"topic_id\":78,\"message_id\":5,\"final_mt_status\":\"complete\"}");
    feed(&b, &m, "299 messageTerminateStatus {\"topic_id\":77,\"final_mt_status\":\"complete\"}");
    feed(&b, &m, "299 messageTerminateStatus {\"topic_id\":77,\"message_id\":6,\"final_mt_status\":\"complete\"}");
    feed(&b, &m, "299 messageTerminateOther {\"topic_id\":77,\"message_id\":5}");
    assert(b.mt_pending && m.event_count == 0);

    /* Only unsolicited lines are transfer events. */
    assert(parse("200 messageTerminateStatus {\"topic_id\":77,\"message_id\":5,"
                 "\"final_mt_status\":\"complete\"}", &f));
    ts_mt_on_frame(&b, &f);
    assert(b.mt_pending && m.event_count == 0);
}

static void test_mt_status_permutations(void)
{
    static const char *const bad_status[] = {
        "299 messageTerminateStatus {\"topic_id\":77,\"message_id\":5}",         /* no status */
        "299 messageTerminateStatus {\"topic_id\":77,\"message_id\":5,"
            "\"final_mt_status\":\"failed\"}",                                    /* not complete */
    };
    uint8_t msg[6];
    size_t i;
    ts_bear9704_t b;
    sim_modem_t m;

    for (i = 0; i < sizeof bad_status / sizeof bad_status[0]; ++i) {
        message_with_crc(msg, true);
        mt_pending(&b, &m);
        feed_segment(&b, &m, 0, msg, 6);
        feed(&b, &m, bad_status[i]);
        assert(!b.mt_pending && !b.mt_available);
        assert(sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_PROTOCOL);
    }

    /* Only one byte received: too short to hold the CRC. */
    mt_pending(&b, &m);
    feed_segment(&b, &m, 0, msg, 1);
    feed(&b, &m, COMPLETE);
    assert(!b.mt_available && sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_PROTOCOL);

    /* A gap before the last segment received. */
    mt_pending(&b, &m);
    feed_segment(&b, &m, 2, msg + 2, 2);
    feed(&b, &m, COMPLETE);
    assert(!b.mt_available && sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_PROTOCOL);

    /* CRC high byte right, low byte wrong. */
    message_with_crc(msg, false);
    mt_pending(&b, &m);
    feed_segment(&b, &m, 0, msg, 6);
    feed(&b, &m, COMPLETE);
    assert(!b.mt_available && sim_saw(&m, TS_EVENT_MT_DROPPED) && m.last_detail == TS_ERR_PROTOCOL);
}
#endif

/* ---- ts_gnss.c ---- */

static uint8_t gnss_bytes[100];
static size_t gnss_pos, gnss_len;
static unsigned gnss_power_calls;

static int gnss_read(void *user, uint8_t *byte)
{
    (void)user;
    if (gnss_pos >= gnss_len) return 0;
    *byte = gnss_bytes[gnss_pos++];
    return 1;
}

static bool gnss_power(void *user, bool on)
{
    (void)user;
    (void)on;
    ++gnss_power_calls;
    return true;
}

static void test_gnss_arguments(void)
{
    ts_gnss_t g;
    ts_gnss_port_t p = {NULL, gnss_read, gnss_power};

    p.read_byte = NULL;                       /* no reader */
    assert(!ts_gnss_init(&g, &p));
    p.read_byte = gnss_read;
    p.power = NULL;                           /* no power control */
    assert(!ts_gnss_init(&g, &p));
    p.power = gnss_power;
    assert(ts_gnss_init(&g, &p) && g.status == TS_GNSS_OFF);

#if TS_ENABLE_GNSS
    gnss_power_calls = 0;
    assert(!ts_gnss_begin(NULL, 0, 10));      /* null context */
    memset(&g, 0, sizeof g);                  /* never initialised: no port */
    assert(!ts_gnss_begin(&g, 0, 10));
    g.port.power = gnss_power;                /* power but no reader */
    assert(!ts_gnss_begin(&g, 0, 10));
    assert(gnss_power_calls == 0 && g.status == TS_GNSS_OFF);

    /* Cancel and poll do nothing unless acquiring. */
    assert(ts_gnss_init(&g, &p));
    ts_gnss_cancel(NULL);
    ts_gnss_poll(NULL, 0);
    gnss_pos = 0;
    gnss_len = 1;
    ts_gnss_cancel(&g);
    ts_gnss_poll(&g, 0);
    assert(g.status == TS_GNSS_OFF && gnss_power_calls == 0 && gnss_pos == 0);
#endif
}

#if TS_ENABLE_GNSS
static void put32(uint8_t *p, int32_t v)
{
    uint32_t u = (uint32_t)v;
    p[0] = (uint8_t)u; p[1] = (uint8_t)(u >> 8); p[2] = (uint8_t)(u >> 16); p[3] = (uint8_t)(u >> 24);
}

/* Builds a checksummed UBX NAV-PVT frame. */
static void nav_pvt(uint8_t fix_type, uint8_t flags, int32_t lon, int32_t lat)
{
    uint8_t a = 0, b = 0;
    unsigned i;
    memset(gnss_bytes, 0, sizeof gnss_bytes);
    gnss_bytes[0] = 0xb5; gnss_bytes[1] = 0x62; gnss_bytes[2] = 1; gnss_bytes[3] = 7;
    gnss_bytes[4] = 92;
    gnss_bytes[6 + 20] = fix_type;
    gnss_bytes[6 + 21] = flags;
    gnss_bytes[6 + 23] = 9;
    put32(gnss_bytes + 6 + 24, lon);
    put32(gnss_bytes + 6 + 28, lat);
    put32(gnss_bytes + 6 + 40, 2500);
    for (i = 2; i < 98; ++i) { a = (uint8_t)(a + gnss_bytes[i]); b = (uint8_t)(b + a); }
    gnss_bytes[98] = a;
    gnss_bytes[99] = b;
    gnss_pos = 0;
    gnss_len = sizeof gnss_bytes;
}

static ts_gnss_status_t acquire(ts_gnss_t *g)
{
    unsigned i;
    ts_gnss_port_t p = {NULL, gnss_read, gnss_power};
    assert(ts_gnss_init(g, &p));
    assert(ts_gnss_begin(g, 0, 1000));
    for (i = 0; i < 4; ++i) ts_gnss_poll(g, 1);
    return g->status;
}

static void test_gnss_frame_permutations(void)
{
    ts_gnss_t g;

    /* South-west fix: negative coordinates decode as negative. */
    nav_pvt(3, 1, -1234567, -7654321);
    assert(acquire(&g) == TS_GNSS_FIX);
    assert(g.fix.longitude_e7 == -1234567 && g.fix.latitude_e7 == -7654321);
    assert(g.fix.satellites == 9 && g.fix.horizontal_accuracy_mm == 2500);
    /* Polling after the fix reads nothing more. */
    gnss_pos = 0;
    ts_gnss_poll(&g, 2);
    assert(gnss_pos == 0 && g.status == TS_GNSS_FIX);

    /* Range limits, just outside: not a fix, nothing reported. */
    nav_pvt(3, 1, 0, -900000001);
    assert(acquire(&g) == TS_GNSS_ACQUIRING && g.fix.latitude_e7 == 0);
    nav_pvt(3, 1, -1800000001, 0);
    assert(acquire(&g) == TS_GNSS_ACQUIRING && g.fix.longitude_e7 == 0);
    nav_pvt(3, 1, 1800000001, 0);
    assert(acquire(&g) == TS_GNSS_ACQUIRING && g.fix.longitude_e7 == 0);
    nav_pvt(3, 1, 1800000000, 900000000);       /* exactly at the limits */
    assert(acquire(&g) == TS_GNSS_FIX);

    /* Checksum A wrong, checksum B wrong, 2D fix, fix flag clear. */
    nav_pvt(3, 1, 1, 2);
    gnss_bytes[98] ^= 1u;
    assert(acquire(&g) == TS_GNSS_ACQUIRING);
    nav_pvt(3, 1, 1, 2);
    gnss_bytes[99] ^= 1u;
    assert(acquire(&g) == TS_GNSS_ACQUIRING);
    nav_pvt(2, 1, 1, 2);
    assert(acquire(&g) == TS_GNSS_ACQUIRING);
    nav_pvt(3, 0, 1, 2);
    assert(acquire(&g) == TS_GNSS_ACQUIRING);
    ts_gnss_cancel(&g);
    assert(g.status == TS_GNSS_OFF);
}
#endif

int main(void)
{
    test_activation_text();
    test_base64_encode_arguments();
    test_base64_decode_permutations();
    test_crc_null_input();
    test_json_member_lookup();
    test_json_nested_values();
    test_json_u32_values();
    test_json_string_values();
    test_json_versions();
    test_jspr_parse_permutations();
    test_setup_waits_while_command_in_flight();
    test_setup_sim_interface_missing();
    test_no_event_callback();
    test_ready_callback_preempts_imei_query();
    test_fault_hold_stops_line_processing();
    test_shutdown_busy_permutations();
    test_send_and_query_arguments();
    test_query_ignores_other_replies();
    test_shutdown_ignores_other_replies();
    test_newline_handling();
    test_mo_accept_reply_permutations();
    test_mo_start_ignores_other_lines();
    test_mo_segment_request_permutations();
    test_mo_active_other_lines();
#if TS_ENABLE_MT
    test_mt_announce_permutations();
    test_mt_announce_while_message_unread();
    test_mt_segment_permutations();
    test_mt_ignores_other_messages();
    test_mt_status_permutations();
#endif
    test_gnss_arguments();
#if TS_ENABLE_GNSS
    test_gnss_frame_permutations();
#endif
    puts("test_branches: all passed");
    return 0;
}
