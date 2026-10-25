/* SPDX-License-Identifier: MIT */
#include "ts_mo_tracker.h"

void ts_mo_init(ts_mo_tracker_t *t)
{
    t->phase = TS_MO_IDLE;
    t->topic = -1;
    t->reference = -1;
    t->message_id = -1;
    t->started_ms = 0;
}

bool ts_mo_begin(ts_mo_tracker_t *t, int topic, int reference, uint32_t now_ms)
{
    if (t->phase != TS_MO_IDLE || topic < 64 || reference < 1 || reference > 100)
        return false;
    ts_mo_init(t);
    t->phase = TS_MO_REQUESTED;
    t->topic = topic;
    t->reference = reference;
    t->started_ms = now_ms;
    return true;
}

static bool is_ours(const ts_mo_tracker_t *t, int message_id)
{
    return message_id >= 0 && (t->message_id < 0 || t->message_id == message_id);
}

bool ts_mo_segment_ok(const ts_mo_tracker_t *t, int topic, int message_id,
                      int start, int length, int message_length,
                      int buffer_capacity)
{
    return t->phase != TS_MO_IDLE && topic == t->topic && is_ours(t, message_id) &&
           start >= 0 && length > 0 && length <= buffer_capacity &&
           start <= message_length && length <= message_length - start;
}

void ts_mo_segment_sent(ts_mo_tracker_t *t, int message_id)
{
    /* The modem can ask for bytes before we hear it accepted the request. */
    if (t->phase != TS_MO_IDLE && is_ours(t, message_id))
        t->message_id = message_id;
}

ts_mo_outcome_t ts_mo_on_status(ts_mo_tracker_t *t, int topic, int reference,
                                int message_id, ts_mo_status_t status)
{
    if (t->phase == TS_MO_IDLE || topic != t->topic ||
        status == TS_MO_STATUS_INVALID)
        return TS_MO_OUTCOME_IGNORED;

    if (status == TS_MO_STATUS_ACCEPTED) {
        /* Acceptance carries our own reference number. */
        if (reference != t->reference || !is_ours(t, message_id))
            return TS_MO_OUTCOME_IGNORED;
        t->phase = TS_MO_ACCEPTED;
        t->message_id = message_id;
        return TS_MO_OUTCOME_ACCEPTED;
    }

    /* Final reports often carry reference 0, so match on the modem's ID. */
    if (t->message_id >= 0) {
        if (message_id != t->message_id) return TS_MO_OUTCOME_IGNORED;
    } else if (reference == 0 || reference != t->reference || message_id < 0) {
        return TS_MO_OUTCOME_IGNORED;
    }
    t->phase = TS_MO_IDLE;
    return status == TS_MO_STATUS_DELIVERED ? TS_MO_OUTCOME_DELIVERED
                                            : TS_MO_OUTCOME_FAILED;
}

bool ts_mo_check_timeout(ts_mo_tracker_t *t, uint32_t now_ms, uint32_t timeout_ms)
{
    /* Unsigned subtraction survives the millisecond counter wrapping. */
    if (t->phase != TS_MO_REQUESTED || (uint32_t)(now_ms - t->started_ms) < timeout_ms)
        return false;
    t->phase = TS_MO_UNCONFIRMED;
    return true;
}

bool ts_mo_unlock(ts_mo_tracker_t *t)
{
    if (t->phase != TS_MO_UNCONFIRMED) return false;
    ts_mo_init(t);
    return true;
}

const char *ts_mo_phase_name(ts_mo_phase_t phase)
{
    switch (phase) {
    case TS_MO_IDLE:        return "IDLE";
    case TS_MO_REQUESTED:   return "REQUESTED (waiting for the modem)";
    case TS_MO_ACCEPTED:    return "ACCEPTED (queued, not yet delivered)";
    case TS_MO_UNCONFIRMED: return "UNCONFIRMED (outcome unknown)";
    }
    return "UNKNOWN";
}
