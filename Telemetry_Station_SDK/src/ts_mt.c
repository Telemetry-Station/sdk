/* SPDX-License-Identifier: MIT */
#include "ts_internal.h"
#include <string.h>

/* Receiving one message (mobile-terminated, "MT"). The modem announces it,
 * sends it in base64 segments (possibly out of order or repeated), then
 * reports a final status. We only expose it once every byte has arrived and
 * the trailing CRC16 checks out. */

#if TS_ENABLE_MT

static bool seen(const ts_bear9704_t *ctx, size_t i)
{
    return (ctx->mt_seen[i >> 3] & (uint8_t)(1u << (i & 7u))) != 0;
}

static void mark_seen(ts_bear9704_t *ctx, size_t i)
{
    ctx->mt_seen[i >> 3] |= (uint8_t)(1u << (i & 7u));
}

static void drop(ts_bear9704_t *ctx, ts_result_t reason)
{
    ctx->mt_pending = false;
    ctx->mt_covered = ctx->mt_max_end = 0;
    memset(ctx->mt_seen, 0, sizeof(ctx->mt_seen));
    ts_emit(ctx, TS_EVENT_MT_DROPPED, reason);
}

static void on_announce(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    uint32_t topic, id, size;
    if (ctx->mt_pending || ctx->mt_available) {
        ts_emit(ctx, TS_EVENT_MT_DROPPED, TS_ERR_BUSY);
        return;
    }
    if (!ts_json_u32(f->json, "topic_id", &topic) || topic < TS_MIN_TOPIC_ID || topic > UINT16_MAX ||
        !ts_json_u32(f->json, "message_id", &id) || id > 65535u ||
        !ts_json_u32(f->json, "message_length_max", &size) || size < 2u ||
        size > TS_MAX_PAYLOAD + 2u) {
        ts_emit(ctx, TS_EVENT_MT_DROPPED, TS_ERR_CAPACITY);
        return;
    }
    ctx->mt_pending = true;
    ctx->mt_topic = (uint16_t)topic;
    ctx->mt_id = (uint16_t)id;
    ctx->mt_length = (uint16_t)size; /* Upper bound until the message is complete. */
    ctx->mt_covered = ctx->mt_max_end = 0;
    memset(ctx->mt_seen, 0, sizeof(ctx->mt_seen));
    ctx->mt_deadline = ts_now(ctx) + TS_MT_TIMEOUT_MS;
}

static void on_segment(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    uint32_t start, size;
    const char *base64;
    size_t base64_len, decoded_len, i;
    uint8_t bytes[TS_MAX_PAYLOAD + 2u];

    if (!ts_json_u32(f->json, "segment_start", &start) ||
        !ts_json_u32(f->json, "segment_length", &size) || !size ||
        start > ctx->mt_length || size > (uint32_t)ctx->mt_length - start ||
        !ts_json_string_span(f->json, "data", &base64, &base64_len) ||
        !ts_base64_decode(base64, base64_len, bytes, sizeof(bytes), &decoded_len) ||
        decoded_len != size) {
        drop(ctx, TS_ERR_PROTOCOL);
        return;
    }
    for (i = 0; i < decoded_len; ++i) {
        size_t at = (size_t)start + i;
        if (!seen(ctx, at)) {
            ctx->mt[at] = bytes[i];
            mark_seen(ctx, at);
            ctx->mt_covered++;
        } else if (ctx->mt[at] != bytes[i]) { /* A repeat must match. */
            drop(ctx, TS_ERR_PROTOCOL);
            return;
        }
    }
    if (ctx->mt_max_end < start + size) ctx->mt_max_end = (uint16_t)(start + size);
    ctx->mt_deadline = ts_now(ctx) + TS_MT_TIMEOUT_MS;
}

static void on_status(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    char status[32];
    uint16_t end = ctx->mt_max_end;
    uint16_t crc;

    if (!ts_json_string(f->json, "final_mt_status", status, sizeof(status)) ||
        strcmp(status, "complete") || end < 2u || ctx->mt_covered != end) {
        drop(ctx, TS_ERR_PROTOCOL);
        return;
    }
    crc = ts_crc16(ctx->mt, (size_t)end - 2u);
    if (ctx->mt[end - 2u] != (uint8_t)(crc >> 8) ||
        ctx->mt[end - 1u] != (uint8_t)(crc & 0xffu)) {
        drop(ctx, TS_ERR_PROTOCOL);
        return;
    }
    ctx->mt_length = (uint16_t)(end - 2u);
    ctx->mt_pending = false;
    ctx->mt_available = true;
    ts_emit(ctx, TS_EVENT_MT_RECEIVED, ctx->mt_length);
}

void ts_mt_on_frame(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    uint32_t topic, id;
    if (f->code != TS_JSPR_UNSOLICITED) return;
    if (!strcmp(f->target, "messageTerminate")) {
        on_announce(ctx, f);
        return;
    }
    if (!ctx->mt_pending) return;
    if (!ts_json_u32(f->json, "topic_id", &topic) || topic != ctx->mt_topic ||
        !ts_json_u32(f->json, "message_id", &id) || id != ctx->mt_id) return;

    if (!strcmp(f->target, "messageTerminateSegment")) on_segment(ctx, f);
    else if (!strcmp(f->target, "messageTerminateStatus")) on_status(ctx, f);
}

void ts_mt_poll(ts_bear9704_t *ctx, uint32_t now)
{
    if (ctx->mt_pending && ts_expired(now, ctx->mt_deadline))
        drop(ctx, TS_ERR_TIMEOUT);
}

#else

void ts_mt_poll(ts_bear9704_t *ctx, uint32_t now)
{
    (void)ctx; (void)now;
}

#endif
