/* SPDX-License-Identifier: MIT */
#ifndef TS_MO_TRACKER_H
#define TS_MO_TRACKER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Keeps track of the one message being sent. It holds only state and makes
 * decisions; it never touches hardware, so it can be tested on a computer.
 *
 *   IDLE -> REQUESTED -> ACCEPTED -> back to IDLE when the result arrives
 *              |
 *              +-> UNCONFIRMED if the modem never answered
 *
 * Only one message may be outstanding. We never retry automatically, because
 * if the outcome is unknown a retry could send (and bill) the message twice.
 */
typedef enum {
    TS_MO_IDLE,
    TS_MO_REQUESTED,
    TS_MO_ACCEPTED,
    TS_MO_UNCONFIRMED
} ts_mo_phase_t;

/* What the modem told us, in terms this module understands. */
typedef enum {
    TS_MO_STATUS_INVALID,
    TS_MO_STATUS_ACCEPTED,
    TS_MO_STATUS_DELIVERED,
    TS_MO_STATUS_FAILED
} ts_mo_status_t;

typedef enum {
    TS_MO_OUTCOME_IGNORED, /* Not about our message. */
    TS_MO_OUTCOME_ACCEPTED,
    TS_MO_OUTCOME_DELIVERED,
    TS_MO_OUTCOME_FAILED
} ts_mo_outcome_t;

typedef struct {
    ts_mo_phase_t phase;
    int topic;
    int reference;  /* Our number for the request, 1 to 100. */
    int message_id; /* The modem's number, -1 until known. */
    uint32_t started_ms;
} ts_mo_tracker_t;

void ts_mo_init(ts_mo_tracker_t *t);

/* Starts a request. False if one is already outstanding or inputs are bad. */
bool ts_mo_begin(ts_mo_tracker_t *t, int topic, int reference, uint32_t now_ms);

/* True if the modem may be given bytes [start, start + length) of a message
 * that is message_length bytes long, into a buffer of buffer_capacity bytes. */
bool ts_mo_segment_ok(const ts_mo_tracker_t *t, int topic, int message_id,
                      int start, int length, int message_length,
                      int buffer_capacity);

/* Call after giving the modem a segment so its message ID is remembered. */
void ts_mo_segment_sent(ts_mo_tracker_t *t, int message_id);

/* Applies a status report from the modem. */
ts_mo_outcome_t ts_mo_on_status(ts_mo_tracker_t *t, int topic, int reference,
                                int message_id, ts_mo_status_t status);

/* True once when the modem has not accepted the request within timeout_ms.
 * The request becomes UNCONFIRMED. */
bool ts_mo_check_timeout(ts_mo_tracker_t *t, uint32_t now_ms, uint32_t timeout_ms);

/* Releases an UNCONFIRMED request. This only clears our local lock. It does
 * not cancel anything the modem or network may still be holding. */
bool ts_mo_unlock(ts_mo_tracker_t *t);

const char *ts_mo_phase_name(ts_mo_phase_t phase);

#ifdef __cplusplus
}
#endif

#endif
