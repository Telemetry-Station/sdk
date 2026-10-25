/* SPDX-License-Identifier: MIT */
#ifndef TS_BEAR9704_H
#define TS_BEAR9704_H

#include "ts_config.h"
#include "ts_platform.h"
#include "ts_version.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Driver for the BEAR Module 9704 (an Iridium 9704 modem board), used from
 * your own microcontroller.
 *
 * Typical use:
 *
 *   ts_bear_init(&bear, &platform, on_event, NULL);
 *   ts_bear_start(&bear);
 *   for (;;) {
 *       ts_bear_poll(&bear);                       // every few milliseconds
 *       if (time_to_report && ts_bear_is_ready(&bear))
 *           ts_bear_send(&bear, TS_TOPIC_ID, bytes, length);
 *   }
 *
 * One message is sent at a time and nothing is retried automatically, because
 * a retry can double-bill a message that actually got through. Put a sequence
 * number in your payload so the cloud can detect duplicates and gaps.
 */

/* IMT topic IDs below this value are reserved; yours is issued at activation. */
#define TS_MIN_TOPIC_ID 64u

/* The driver's state. Read it with ts_bear_state(). */
typedef enum {
    TS_OFF = 0,
    TS_BOOTING,
    TS_API_QUERY,
    TS_API_SELECT,
    TS_SIM_QUERY,
    TS_SIM_SET,
    TS_MODE_QUERY,
    TS_MODE_SET,
    TS_READY,
    TS_MO_START,
    TS_MO_ACTIVE,
    TS_SHUTDOWN_WAIT,
    TS_POWERING_DOWN, /* Waiting for the module's booted signal to fall. */
    TS_FAULT
} ts_state_t;

/* Events reported to the callback given to ts_bear_init(). */
typedef enum {
    TS_EVENT_READY = 1,    /* Modem is up; you may send. */
    TS_EVENT_MO_ACCEPTED,  /* Modem queued the message. Not yet delivered. */
    TS_EVENT_MO_DELIVERED, /* Iridium confirmed receipt. */
    TS_EVENT_MO_FAILED,    /* detail is a ts_result_t. Outcome may be unknown. */
    TS_EVENT_MT_RECEIVED,  /* Call ts_bear_take_mt(). detail is the length. */
    TS_EVENT_MT_DROPPED,
    TS_EVENT_SIGNAL,       /* Read ts_bear_signal_bars(). */
    TS_EVENT_IMEI,         /* Read ts_bear_imei(). */
    TS_EVENT_STOPPED,      /* Modem is powered off. */
    TS_EVENT_ERROR         /* detail is a ts_result_t. */
} ts_event_t;

typedef enum {
    TS_OK = 0,
    TS_ERR_ARGUMENT = -1,
    TS_ERR_BUSY = -2,
    TS_ERR_CAPACITY = -3,
    TS_ERR_IO = -4,
    TS_ERR_PROTOCOL = -5,
    TS_ERR_TIMEOUT = -6,
    TS_ERR_REJECTED = -7,
    TS_ERR_NOT_READY = -8
} ts_result_t;

/* The one information request that may be outstanding while TS_READY. */
typedef enum {
    TS_QUERY_NONE = 0,
    TS_QUERY_SIGNAL,
    TS_QUERY_IMEI
} ts_query_t;

typedef struct ts_bear9704 ts_bear9704_t;
typedef void (*ts_event_fn)(void *user, ts_event_t event, int32_t detail);

/* Declared here so you can allocate it statically (no malloc). Treat the
 * fields as private and use the functions below. */
struct ts_bear9704 {
    ts_platform_t port;
    ts_event_fn event_cb;
    void *event_user;
    ts_state_t state;
    uint32_t deadline;
    uint32_t mt_deadline;
    ts_state_t down_final; /* State to settle in once powered down. */
    ts_query_t query;      /* Information request awaiting its reply. */
    uint16_t topic;
    uint16_t message_id;
    uint8_t request_reference;
    bool mo_pending;
    bool mt_pending;
    bool mt_available;
    uint16_t mo_length;
    uint16_t mt_length;
    uint16_t mt_max_end;
    uint16_t mt_covered;
    uint16_t mt_topic;
    uint16_t mt_id;
    int8_t signal_bars;
    char imei[16];
    uint16_t tx_len;
    uint16_t tx_pos;
    uint16_t rx_len;
    bool rx_discard;
    char tx[TS_LINE_CAP];
    char rx[TS_LINE_CAP];
    uint8_t mo[TS_MAX_PAYLOAD + 2u]; /* Payload followed by its CRC16. */
#if TS_ENABLE_MT
    uint8_t mt[TS_MAX_PAYLOAD + 2u];
    uint8_t mt_seen[(TS_MAX_PAYLOAD + 9u) / 8u];
#endif
};

/* Does not power the modem. Fails if the platform is missing a function. */
ts_result_t ts_bear_init(ts_bear9704_t *ctx, const ts_platform_t *port,
                         ts_event_fn cb, void *cb_user);

/* Powers the modem and begins start-up. Wait for TS_EVENT_READY. */
ts_result_t ts_bear_start(ts_bear9704_t *ctx);

/* Asks the modem to stop, then powers it off (TS_EVENT_STOPPED). */
ts_result_t ts_bear_shutdown(ts_bear9704_t *ctx);

/* Does all the work. Call it every few milliseconds from your main loop,
 * never from an interrupt. */
void ts_bear_poll(ts_bear9704_t *ctx);

bool ts_bear_is_ready(const ts_bear9704_t *ctx);

/* The current state. TS_OFF and TS_FAULT mean the module is powered down and
 * it is safe to call ts_bear_start() again. */
ts_state_t ts_bear_state(const ts_bear9704_t *ctx);

/* Queues one message. The topic must be TS_MIN_TOPIC_ID or higher. The result of the
 * transmission arrives later as events. */
ts_result_t ts_bear_send(ts_bear9704_t *ctx, uint16_t topic,
                         const uint8_t *data, size_t length);

/* Copies out a received message (CRC already verified and removed). */
ts_result_t ts_bear_take_mt(ts_bear9704_t *ctx, uint8_t *out,
                            size_t capacity, size_t *length, uint16_t *topic);

/* Asynchronous queries, answered by TS_EVENT_SIGNAL and TS_EVENT_IMEI.
 * Only one may be outstanding, and not while a message is being sent. */
ts_result_t ts_bear_request_signal(ts_bear9704_t *ctx);
ts_result_t ts_bear_request_imei(ts_bear9704_t *ctx);

/* NULL until the modem has reported its IMEI. */
const char *ts_bear_imei(const ts_bear9704_t *ctx);

/* 0 to 5 bars, or -1 until the first signal report. */
int8_t ts_bear_signal_bars(const ts_bear9704_t *ctx);

/* Short names for logging, for example "TS_READY". They live in ts_names.c;
 * leave that file out of your build if you do not log. */
const char *ts_state_name(ts_state_t state);
const char *ts_event_name(ts_event_t event);
const char *ts_result_name(ts_result_t result);

#ifdef __cplusplus
}
#endif
#endif
