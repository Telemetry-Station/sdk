/* SPDX-License-Identifier: MIT */
#include "ts_internal.h"
#include <string.h>

/* Builds the command lines sent to the modem. Everything here is bounded by
 * the size of the driver's transmit buffer; nothing uses stdio. */

bool ts_tx_busy(const ts_bear9704_t *ctx)
{
    return ctx->tx_pos < ctx->tx_len;
}

bool ts_cmd_begin(ts_bear9704_t *ctx, ts_cmd_t *cmd)
{
    if (ts_tx_busy(ctx)) return false;
    cmd->buf = ctx->tx;
    cmd->cap = sizeof(ctx->tx);
    cmd->len = 0;
    cmd->overflow = false;
    return true;
}

void ts_cmd_str(ts_cmd_t *cmd, const char *text)
{
    size_t n = strlen(text);
    if (cmd->overflow || n >= cmd->cap - cmd->len) {
        cmd->overflow = true;
        return;
    }
    memcpy(cmd->buf + cmd->len, text, n);
    cmd->len += n;
}

void ts_cmd_u32(ts_cmd_t *cmd, uint32_t value)
{
    char digits[10]; /* 4294967295 has ten digits. */
    size_t n = 0;
    do {
        digits[n++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    if (cmd->overflow || n >= cmd->cap - cmd->len) {
        cmd->overflow = true;
        return;
    }
    while (n) cmd->buf[cmd->len++] = digits[--n];
}

/* Encodes straight into the transmit buffer, so no second copy is needed. */
void ts_cmd_base64(ts_cmd_t *cmd, const uint8_t *data, size_t length)
{
    size_t n;
    if (cmd->overflow) return;
    n = ts_base64_encode(data, length, cmd->buf + cmd->len,
                         cmd->cap - cmd->len);
    if (!n) cmd->overflow = true;
    else cmd->len += n;
}

bool ts_cmd_end(ts_bear9704_t *ctx, const ts_cmd_t *cmd)
{
    if (cmd->overflow || cmd->len == 0) return false;
    ctx->tx_len = (uint16_t)cmd->len;
    ctx->tx_pos = 0;
    ts_set_deadline(ctx, TS_CMD_TIMEOUT_MS);
    return true;
}

bool ts_cmd(ts_bear9704_t *ctx, const char *command)
{
    ts_cmd_t cmd;
    if (!ts_cmd_begin(ctx, &cmd)) return false;
    ts_cmd_str(&cmd, command);
    return ts_cmd_end(ctx, &cmd);
}
