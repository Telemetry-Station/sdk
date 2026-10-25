/* SPDX-License-Identifier: MIT */
#ifndef TS_INTERNAL_H
#define TS_INTERNAL_H

#include "ts_bear9704.h"

/* Private to the SDK. Applications should only include public headers. */

/* ---- Modem wire protocol (JSPR) status codes ---- */
#define TS_JSPR_CODE_MIN 200u      /* Lowest valid code. */
#define TS_JSPR_OK 200u            /* A request succeeded. */
#define TS_JSPR_UNSOLICITED 299u   /* The modem reports a transfer event. */
#define TS_JSPR_ERROR_FIRST 400u   /* 400 and above: request refused. */
#define TS_JSPR_CODE_MAX 599u      /* Highest valid code. */

/* Request references cycle through 1..TS_MAX_REQUEST_REFERENCE. */
#define TS_MAX_REQUEST_REFERENCE 100u

/* ---- Modem wire protocol (JSPR): text lines such as
 *      200 apiVersion {"active_version":null}
 *      299 messageOriginateStatus {...}
 *      and their JSON fields. ---- */
typedef struct {
    uint16_t code;
    char target[32];
    const char *json; /* Points into the receive buffer; valid until next poll. */
} ts_jspr_frame_t;

bool ts_jspr_parse(char *line, ts_jspr_frame_t *frame);
bool ts_json_u32(const char *json, const char *key, uint32_t *value);
bool ts_json_string(const char *json, const char *key, char *out, size_t cap);
bool ts_json_string_span(const char *json, const char *key,
                         const char **data, size_t *length);
bool ts_json_version(const char *json, const char *field,
                     uint32_t *major, uint32_t *minor, uint32_t *patch);
bool ts_json_first_version(const char *json,
                           uint32_t *major, uint32_t *minor, uint32_t *patch);

/* ---- Encodings ---- */
uint16_t ts_crc16(const uint8_t *data, size_t length);
size_t ts_base64_encode(const uint8_t *in, size_t len, char *out, size_t cap);
bool ts_base64_decode(const char *in, size_t len, uint8_t *out,
                      size_t cap, size_t *written);

/* ---- Driver helpers shared by the state handlers ---- */
uint32_t ts_now(const ts_bear9704_t *ctx);
bool ts_expired(uint32_t now, uint32_t deadline);
void ts_set_deadline(ts_bear9704_t *ctx, uint32_t delay_ms);
void ts_emit(ts_bear9704_t *ctx, ts_event_t event, int32_t detail);
bool ts_tx_busy(const ts_bear9704_t *ctx);

/* Sends a fixed command. False if the previous command is still going out. */
bool ts_cmd(ts_bear9704_t *ctx, const char *command);

/* Builds a command in the transmit buffer without stdio:
 *
 *   ts_cmd_t c;
 *   if (ts_cmd_begin(ctx, &c)) {
 *       ts_cmd_str(&c, "PUT x {\"n\":");
 *       ts_cmd_u32(&c, 5u);
 *       ts_cmd_str(&c, "}\r");
 *       ok = ts_cmd_end(ctx, &c);
 *   }
 *
 * Overflow is remembered and reported by ts_cmd_end(). */
typedef struct {
    char *buf;
    size_t cap;
    size_t len;
    bool overflow;
} ts_cmd_t;

bool ts_cmd_begin(ts_bear9704_t *ctx, ts_cmd_t *cmd);
void ts_cmd_str(ts_cmd_t *cmd, const char *text);
void ts_cmd_u32(ts_cmd_t *cmd, uint32_t value);
void ts_cmd_base64(ts_cmd_t *cmd, const uint8_t *data, size_t length);
bool ts_cmd_end(ts_bear9704_t *ctx, const ts_cmd_t *cmd);
void ts_fail(ts_bear9704_t *ctx, ts_result_t reason);
void ts_set_ready(ts_bear9704_t *ctx);

/* ---- State handlers, one file each ---- */
/* Start-up handshake. Returns false on an unexpected reply. */
bool ts_setup_on_frame(ts_bear9704_t *ctx, const ts_jspr_frame_t *frame);

/* Sending a message. */
void ts_mo_on_frame(ts_bear9704_t *ctx, const ts_jspr_frame_t *frame);
void ts_mo_abort(ts_bear9704_t *ctx, ts_result_t reason);

/* Receiving a message. */
#if TS_ENABLE_MT
void ts_mt_on_frame(ts_bear9704_t *ctx, const ts_jspr_frame_t *frame);
#endif
void ts_mt_poll(ts_bear9704_t *ctx, uint32_t now);

#endif
