/* SPDX-License-Identifier: MIT */
#include "ts_internal.h"
#include <string.h>

/* Sending one message (mobile-originated, "MO"):
 *   PUT messageOriginate         -> modem replies "message_accepted" + id
 *   299 messageOriginateSegment  -> modem asks for a range of bytes
 *   PUT messageOriginateSegment  -> we answer with those bytes, base64 encoded
 *   299 messageOriginateStatus   -> final result from the Iridium network
 */

void ts_mo_abort(ts_bear9704_t *ctx, ts_result_t reason)
{
    ctx->mo_pending = false;
    ctx->state = TS_READY;
    ts_emit(ctx, TS_EVENT_MO_FAILED, reason);
}

static bool same_message(const ts_bear9704_t *ctx, const char *json)
{
    uint32_t topic, id;
    return ts_json_u32(json, "topic_id", &topic) && topic == ctx->topic &&
           ts_json_u32(json, "message_id", &id) && id == ctx->message_id;
}

static void on_accept_reply(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    uint32_t topic, id;
    char response[32];
    if (f->code != TS_JSPR_OK ||
        !ts_json_string(f->json, "message_response", response, sizeof(response)) ||
        strcmp(response, "message_accepted") ||
        !ts_json_u32(f->json, "message_id", &id) || id > 65535u ||
        !ts_json_u32(f->json, "topic_id", &topic) || topic != ctx->topic) {
        ts_mo_abort(ctx, TS_ERR_REJECTED);
        return;
    }
    ctx->message_id = (uint16_t)id;
    ctx->state = TS_MO_ACTIVE;
    ts_set_deadline(ctx, TS_MO_TIMEOUT_MS);
    ts_emit(ctx, TS_EVENT_MO_ACCEPTED, (int32_t)id);
}

/* The modem asked for bytes [start, start + size) of the message. */
static void on_segment_request(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    uint32_t start, size;
    ts_cmd_t cmd;

    if (!same_message(ctx, f->json) ||
        !ts_json_u32(f->json, "segment_start", &start) ||
        !ts_json_u32(f->json, "segment_length", &size) || !size ||
        start > ctx->mo_length || size > (uint32_t)ctx->mo_length - start) {
        ts_mo_abort(ctx, TS_ERR_PROTOCOL);
        return;
    }
    if (!ts_cmd_begin(ctx, &cmd)) { /* Previous command still being sent. */
        ts_fail(ctx, TS_ERR_PROTOCOL);
        return;
    }
    ts_cmd_str(&cmd, "PUT messageOriginateSegment {\"topic_id\":");
    ts_cmd_u32(&cmd, ctx->topic);
    ts_cmd_str(&cmd, ",\"message_id\":");
    ts_cmd_u32(&cmd, ctx->message_id);
    ts_cmd_str(&cmd, ",\"segment_length\":");
    ts_cmd_u32(&cmd, size);
    ts_cmd_str(&cmd, ",\"segment_start\":");
    ts_cmd_u32(&cmd, start);
    ts_cmd_str(&cmd, ",\"data\":\"");
    ts_cmd_base64(&cmd, ctx->mo + start, (size_t)size);
    ts_cmd_str(&cmd, "\"}\r");
    if (!ts_cmd_end(ctx, &cmd)) ts_mo_abort(ctx, TS_ERR_CAPACITY);
}

static void on_final_status(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    char status[32];
    if (!same_message(ctx, f->json)) return;
    ctx->mo_pending = false;
    ctx->state = TS_READY;
    if (ts_json_string(f->json, "final_mo_status", status, sizeof(status)) &&
        !strcmp(status, "mo_ack_received"))
        ts_emit(ctx, TS_EVENT_MO_DELIVERED, (int32_t)ctx->message_id);
    else
        ts_emit(ctx, TS_EVENT_MO_FAILED, TS_ERR_REJECTED);
}

void ts_mo_on_frame(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    if (!ctx->mo_pending) return;

    if (ctx->state == TS_MO_START) {
        if (!strcmp(f->target, "messageOriginate") && f->code != TS_JSPR_UNSOLICITED)
            on_accept_reply(ctx, f);
        return;
    }
    if (ctx->state != TS_MO_ACTIVE) return;

    if (!strcmp(f->target, "messageOriginateSegment")) {
        if (f->code == TS_JSPR_UNSOLICITED) on_segment_request(ctx, f);
        else if (f->code >= TS_JSPR_ERROR_FIRST) ts_mo_abort(ctx, TS_ERR_REJECTED);
    } else if (f->code == TS_JSPR_UNSOLICITED && !strcmp(f->target, "messageOriginateStatus")) {
        on_final_status(ctx, f);
    }
}
