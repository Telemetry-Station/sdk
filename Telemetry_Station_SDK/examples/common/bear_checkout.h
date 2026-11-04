/* SPDX-License-Identifier: MIT */
#ifndef BEAR_CHECKOUT_H
#define BEAR_CHECKOUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ts_bear9704.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BEAR Module checkout: runs one complete modem cycle (power on, handshake,
 * IMEI, signal, optional send, power down) and prints a PASS/WARN/CHECK report on a
 * text console. The first cycle prints the full report; later cycles print
 * one summary line each, so a long bench run doubles as a soak test.
 *
 *   bear_checkout_init(&ck, &bear, &config);
 *   ...
 *   ts_bear_poll(&bear);
 *   if (time_for_a_cycle) bear_checkout_begin(&ck, now, payload, length);
 *   if (bear_checkout_poll(&ck, now)) { cycle finished and printed }
 *
 * Forward every driver event to bear_checkout_on_event(). Output goes through
 * config.write one whole line at a time; when the console is busy the line is
 * kept and retried on the next poll, so nothing is dropped and nothing blocks.
 * Uses no stdio and no heap.
 */

typedef enum {
    BC_WAIT = 0, /* Not started yet. */
    BC_RUN,      /* In progress (shown as WAIT while it takes a while). */
    BC_PASS,
    BC_WARN,     /* Worked, but needs attention (for example no satellites). */
    BC_FAIL,
    BC_SKIP      /* Not applicable or blocked by an earlier failure. */
} bc_status_t;

typedef enum {
    BC_MODULE_OFF = 0, /* BOOTED low before power-on. */
    BC_BOOT,           /* BOOTED rises after ENABLE. */
    BC_LINK,           /* The modem answers on the UART. */
    BC_SIM,            /* Internal SIM selected. */
    BC_ACTIVE,         /* Radio switched to its active state. */
    BC_IMEI,
    BC_SIGNAL,
    BC_SEND,           /* Modem accepted the message. */
    BC_DELIVERY,       /* Iridium confirmed the message. */
    BC_SHUTDOWN,       /* BOOTED fell after the shutdown request. */
    BC_CHECK_COUNT
} bc_check_t;

/* Writes text to the console. All or nothing: return false (writing nothing)
 * if it does not fit right now. */
typedef bool (*bc_write_fn)(void *user, const char *text);

typedef struct {
    bc_write_fn write;
    void *write_user;
    const char *board;    /* Header text, for example "MSP430F169, XT2 8 MHz". */
    const char *wiring;   /* Header text, for example "ENABLE P1.0, BOOTED P1.1". */
    const char *console;  /* Header text, for example "115200 8N1 on P3.4". */
    uint16_t topic;       /* Below TS_MIN_TOPIC_ID: the send checks are skipped. */
    uint32_t interval_ms; /* Time between cycles, shown in the report. */
    bool color;           /* ANSI colours for the status tags. */
    bool verbose_all;     /* Full report every cycle, not only the first. */
} bc_config_t;

typedef enum { BC_CYCLE_PASS = 0, BC_CYCLE_WARN, BC_CYCLE_FAIL } bc_verdict_t;

#define BC_LINE_CAP 128u

/* Allocate statically. Treat the fields as private. */
typedef struct {
    bc_config_t cfg;
    ts_bear9704_t *bear;
    const uint8_t *payload;
    uint16_t payload_len;
    uint8_t status[BC_CHECK_COUNT];
    int32_t value[BC_CHECK_COUNT];  /* Elapsed ms, bars, message id ... */
    int32_t reason[BC_CHECK_COUNT]; /* ts_result_t behind a CHECK. */
    uint32_t mark[BC_CHECK_COUNT];  /* When each check started. */
    uint32_t cycle_start;
    uint32_t heartbeat_at;
    int32_t last_error;
    uint16_t cycle;
    uint16_t cycles_passed;
    uint16_t mt_length;
    uint16_t mt_topic;
    uint8_t mt_dropped;
    uint8_t phase;
    uint8_t row;      /* Next check to print. */
    uint8_t cursor;   /* Position within header/summary/next steps. */
    uint8_t out;      /* Which output section is printing. */
    uint8_t pending;  /* What the buffered line will advance when written. */
    bool running;
    bool verbose;
    bool asked;
    bool got_reply;
    bool sent;
    bool mt_note;
    int8_t bars;
    uint16_t len;
    char line[BC_LINE_CAP];
} bear_checkout_t;

void bear_checkout_init(bear_checkout_t *ck, ts_bear9704_t *bear,
                        const bc_config_t *config);

/* Starts a cycle. payload may be NULL (or the topic unset) to skip sending.
 * The payload must stay valid until the cycle ends. Returns false if a cycle
 * is already running. */
bool bear_checkout_begin(bear_checkout_t *ck, uint32_t now,
                         const uint8_t *payload, size_t length);

/* Call from your driver event callback. */
void bear_checkout_on_event(bear_checkout_t *ck, ts_event_t event,
                            int32_t detail);

/* Call after every ts_bear_poll(). Returns true once, when the cycle has
 * finished and its report has been completely written. */
bool bear_checkout_poll(bear_checkout_t *ck, uint32_t now);

bool bear_checkout_running(const bear_checkout_t *ck);
bc_status_t bear_checkout_status(const bear_checkout_t *ck, bc_check_t check);
bc_verdict_t bear_checkout_verdict(const bear_checkout_t *ck);

/* True if the cycle's message was accepted by the modem (sequence used). */
bool bear_checkout_sent(const bear_checkout_t *ck);

const char *bear_checkout_label(bc_check_t check);
/* Plain-language text for a ts_result_t failure reason. */
const char *bear_checkout_reason(int32_t reason);

#ifdef __cplusplus
}
#endif
#endif
