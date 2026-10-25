/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_payload.h"
#include "ts_activation.h"
#include "ts_config.h"
#include "ts_mo_tracker.h"

#define TOPIC 394

static void test_normal_send(void)
{
    ts_mo_tracker_t t;
    ts_mo_init(&t);
    assert(ts_mo_begin(&t, TOPIC, 5, 1000));
    assert(!ts_mo_begin(&t, TOPIC, 6, 1000)); /* One at a time. */

    /* The modem asks for bytes before announcing acceptance. */
    assert(ts_mo_segment_ok(&t, TOPIC, 42, 0, 10, 10, 64));
    ts_mo_segment_sent(&t, 42);
    assert(t.message_id == 42);
    assert(!ts_mo_segment_ok(&t, TOPIC, 43, 0, 10, 10, 64)); /* Other message. */
    assert(!ts_mo_segment_ok(&t, TOPIC + 1, 42, 0, 10, 10, 64));
    assert(!ts_mo_segment_ok(&t, TOPIC, 42, 5, 10, 10, 64));  /* Past the end. */
    assert(!ts_mo_segment_ok(&t, TOPIC, 42, 0, 10, 10, 4));   /* Buffer too small. */

    assert(ts_mo_on_status(&t, TOPIC, 5, 42, TS_MO_STATUS_ACCEPTED) == TS_MO_OUTCOME_ACCEPTED);
    assert(t.phase == TS_MO_ACCEPTED);
    /* Final reports often carry reference 0. */
    assert(ts_mo_on_status(&t, TOPIC, 0, 99, TS_MO_STATUS_DELIVERED) == TS_MO_OUTCOME_IGNORED);
    assert(ts_mo_on_status(&t, TOPIC, 0, 42, TS_MO_STATUS_DELIVERED) == TS_MO_OUTCOME_DELIVERED);
    assert(t.phase == TS_MO_IDLE);
}

static void test_failure_and_foreign_status(void)
{
    ts_mo_tracker_t t;
    ts_mo_init(&t);
    assert(ts_mo_on_status(&t, TOPIC, 5, 1, TS_MO_STATUS_ACCEPTED) == TS_MO_OUTCOME_IGNORED);
    assert(!ts_mo_begin(&t, 10, 5, 0));    /* Topic below 64. */
    assert(!ts_mo_begin(&t, TOPIC, 0, 0)); /* Reference out of range. */
    assert(ts_mo_begin(&t, TOPIC, 5, 0));
    assert(ts_mo_on_status(&t, TOPIC, 6, 1, TS_MO_STATUS_ACCEPTED) == TS_MO_OUTCOME_IGNORED);
    assert(ts_mo_on_status(&t, TOPIC, 5, 1, TS_MO_STATUS_ACCEPTED) == TS_MO_OUTCOME_ACCEPTED);
    assert(ts_mo_on_status(&t, TOPIC, 0, 1, TS_MO_STATUS_FAILED) == TS_MO_OUTCOME_FAILED);
    assert(t.phase == TS_MO_IDLE);
}

static void test_timeout_and_unlock(void)
{
    ts_mo_tracker_t t;
    ts_mo_init(&t);
    assert(!ts_mo_unlock(&t));
    assert(ts_mo_begin(&t, TOPIC, 1, 0xFFFFFF00u)); /* Clock about to wrap. */
    assert(!ts_mo_check_timeout(&t, 0xFFFFFF10u, 60000u));
    assert(ts_mo_check_timeout(&t, 60000u, 60000u));
    assert(t.phase == TS_MO_UNCONFIRMED);
    assert(!ts_mo_check_timeout(&t, 120000u, 60000u)); /* Warns only once. */
    assert(!ts_mo_begin(&t, TOPIC, 2, 0));            /* Locked until released. */
    assert(ts_mo_unlock(&t) && t.phase == TS_MO_IDLE);
}

static void test_payload(void)
{
    uint8_t out[TS_MAX_PAYLOAD];
    size_t bytes = 0, bits = 0;
    app_reading_t reading = app_demo_reading(4097u, 3812, 4);

    assert(reading.sequence == 1u); /* The sample schema holds 12 bits. */
    assert(app_payload_encode(&reading, out, sizeof out, &bytes, &bits));
    assert(bytes > 0 && bytes <= TS_MAX_PAYLOAD && bits <= bytes * 8u);
    assert(!app_payload_encode(&reading, out, 2, &bytes, &bits));

    reading.device_id[sizeof reading.device_id - 1] = 'x'; /* No terminator. */
    memset(reading.device_id, 'x', sizeof reading.device_id);
    assert(!app_payload_encode(&reading, out, sizeof out, &bytes, &bits));
}

static void test_activation(void)
{
    char text[300];
    assert(ts_imei_is_valid("300234010753370"));
    assert(!ts_imei_is_valid("12345"));
    assert(ts_activation_text("300234010753370", 21, text, sizeof text) > 0);
    assert(strstr(text, "payloadBytes=21") && strstr(text, TS_ACTIVATE_URL));
}

int main(void)
{
    test_normal_send();
    test_failure_and_foreign_status();
    test_timeout_and_unlock();
    test_payload();
    test_activation();
    puts("PASS: kit logic tests");
    return 0;
}
