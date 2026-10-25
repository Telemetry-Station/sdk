/* SPDX-License-Identifier: MIT */
#include "ts_internal.h"
#include <string.h>

/* The driver runs entirely in the caller's main loop: ts_bear_poll() moves
 * bytes between the platform UART and the line buffers, hands each complete
 * line to the handler for the current state, and checks timeouts. */

/* ---- Helpers shared with the state handlers ---- */

uint32_t ts_now(const ts_bear9704_t *ctx)
{
    return ctx->port.millis(ctx->port.user);
}

bool ts_expired(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

void ts_set_deadline(ts_bear9704_t *ctx, uint32_t delay_ms)
{
    ctx->deadline = ts_now(ctx) + delay_ms;
}

void ts_emit(ts_bear9704_t *ctx, ts_event_t event, int32_t detail)
{
    if (ctx->event_cb) ctx->event_cb(ctx->event_user, event, detail);
}

/* The module must finish shutting down before it is enabled again, so we
 * drive it off and then wait for its "booted" signal to fall (see
 * wait_for_power_down). `final` is the state to settle in afterwards. */
static void power_off(ts_bear9704_t *ctx, ts_state_t final)
{
    ctx->port.uart_enable(ctx->port.user, false);
    ctx->port.power_stop(ctx->port.user);
    ctx->tx_len = ctx->tx_pos = ctx->rx_len = 0;
    ctx->rx_discard = false;
    ctx->down_final = final;
    ctx->state = TS_POWERING_DOWN;
    ts_set_deadline(ctx, TS_BOOT_TIMEOUT_MS);
}

static void wait_for_power_down(ts_bear9704_t *ctx)
{
    bool down = !ctx->port.boot_ready(ctx->port.user);
    if (!down && !ts_expired(ts_now(ctx), ctx->deadline)) return;
    ctx->state = ctx->down_final;
    if (ctx->state == TS_OFF && down) ts_emit(ctx, TS_EVENT_STOPPED, 0);
    else if (ctx->state == TS_OFF) { /* Never reported down: treat as a fault. */
        ctx->state = TS_FAULT;
        ts_emit(ctx, TS_EVENT_ERROR, TS_ERR_TIMEOUT);
    }
}

/* Unrecoverable: power down, and report any message that was in flight. */
void ts_fail(ts_bear9704_t *ctx, ts_result_t reason)
{
    bool was_sending = ctx->mo_pending;
    ctx->mo_pending = false;
    ctx->mt_pending = false;
    ctx->query = TS_QUERY_NONE;
    power_off(ctx, TS_FAULT);
    if (was_sending) ts_emit(ctx, TS_EVENT_MO_FAILED, reason);
    ts_emit(ctx, TS_EVENT_ERROR, reason);
}

void ts_set_ready(ts_bear9704_t *ctx)
{
    ctx->state = TS_READY;
    ctx->query = TS_QUERY_NONE;
    ts_emit(ctx, TS_EVENT_READY, 0);
#if TS_QUERY_IMEI_ON_READY
    if (ctx->state == TS_READY && !ctx->query) (void)ts_bear_request_imei(ctx);
#endif
}

/* ---- Public API ---- */

ts_result_t ts_bear_init(ts_bear9704_t *ctx, const ts_platform_t *port,
                         ts_event_fn cb, void *cb_user)
{
    if (!ctx || !port || !port->serial_read || !port->serial_write ||
        !port->millis || !port->power_start || !port->power_stop ||
        !port->uart_enable || !port->boot_ready) return TS_ERR_ARGUMENT;
    memset(ctx, 0, sizeof(*ctx));
    ctx->port = *port;
    ctx->event_cb = cb;
    ctx->event_user = cb_user;
    ctx->request_reference = 1;
    ctx->signal_bars = -1;
    return TS_OK;
}

ts_result_t ts_bear_start(ts_bear9704_t *ctx)
{
    if (!ctx) return TS_ERR_ARGUMENT;
    if (ctx->state != TS_OFF && ctx->state != TS_FAULT) return TS_ERR_BUSY;
    ctx->mo_pending = ctx->mt_pending = ctx->mt_available = false;
    ctx->rx_len = ctx->tx_len = ctx->tx_pos = 0;
    ctx->rx_discard = false;
    ctx->port.uart_enable(ctx->port.user, false);
    if (!ctx->port.power_start(ctx->port.user)) return TS_ERR_IO;
    ctx->state = TS_BOOTING;
    ts_set_deadline(ctx, TS_BOOT_TIMEOUT_MS);
    return TS_OK;
}

ts_result_t ts_bear_shutdown(ts_bear9704_t *ctx)
{
    if (!ctx) return TS_ERR_ARGUMENT;
    if (ctx->state == TS_OFF) return TS_OK;
    if (ctx->state != TS_READY || ctx->query || ctx->mt_pending ||
        ctx->mt_available) return TS_ERR_BUSY;
    if (!ts_cmd(ctx, "PUT operationalState {\"state\":\"inactive\"}\r"))
        return TS_ERR_IO;
    ctx->state = TS_SHUTDOWN_WAIT;
    return TS_OK;
}

bool ts_bear_is_ready(const ts_bear9704_t *ctx)
{
    return ctx && ctx->state == TS_READY;
}

ts_state_t ts_bear_state(const ts_bear9704_t *ctx)
{
    return ctx ? ctx->state : TS_OFF;
}

static bool send_originate(ts_bear9704_t *ctx)
{
    ts_cmd_t cmd;
    if (!ts_cmd_begin(ctx, &cmd)) return false;
    ts_cmd_str(&cmd, "PUT messageOriginate {\"topic_id\":");
    ts_cmd_u32(&cmd, ctx->topic);
    ts_cmd_str(&cmd, ",\"message_length\":");
    ts_cmd_u32(&cmd, ctx->mo_length);
    ts_cmd_str(&cmd, ",\"request_reference\":");
    ts_cmd_u32(&cmd, ctx->request_reference);
    ts_cmd_str(&cmd, "}\r");
    return ts_cmd_end(ctx, &cmd);
}

ts_result_t ts_bear_send(ts_bear9704_t *ctx, uint16_t topic,
                         const uint8_t *data, size_t length)
{
    uint16_t crc;
    if (!ctx || !data || !length) return TS_ERR_ARGUMENT;
    if (ctx->state != TS_READY || ctx->query) return TS_ERR_NOT_READY;
    if (topic < TS_MIN_TOPIC_ID || length > TS_MAX_PAYLOAD) return TS_ERR_CAPACITY;

    memcpy(ctx->mo, data, length);
    crc = ts_crc16(ctx->mo, length);
    ctx->mo[length] = (uint8_t)(crc >> 8);
    ctx->mo[length + 1u] = (uint8_t)(crc & 0xffu);
    ctx->mo_length = (uint16_t)(length + 2u);
    ctx->topic = topic;

    if (!send_originate(ctx)) return TS_ERR_IO;
    if (++ctx->request_reference > TS_MAX_REQUEST_REFERENCE) ctx->request_reference = 1u;
    ctx->state = TS_MO_START;
    ctx->mo_pending = true;
    return TS_OK;
}

ts_result_t ts_bear_take_mt(ts_bear9704_t *ctx, uint8_t *out,
                            size_t capacity, size_t *length, uint16_t *topic)
{
#if TS_ENABLE_MT
    if (!ctx || !out || !length || !topic) return TS_ERR_ARGUMENT;
    if (!ctx->mt_available) return TS_ERR_NOT_READY;
    if (capacity < ctx->mt_length) return TS_ERR_CAPACITY;
    memcpy(out, ctx->mt, ctx->mt_length);
    *length = ctx->mt_length;
    *topic = ctx->mt_topic;
    ctx->mt_available = false;
    ctx->mt_length = 0;
    return TS_OK;
#else
    (void)ctx; (void)out; (void)capacity; (void)length; (void)topic;
    return TS_ERR_NOT_READY;
#endif
}

static ts_result_t start_query(ts_bear9704_t *ctx, ts_query_t query,
                               const char *command)
{
    if (!ctx || ctx->state != TS_READY || ctx->query) return TS_ERR_NOT_READY;
    if (!ts_cmd(ctx, command)) return TS_ERR_BUSY;
    ctx->query = query;
    return TS_OK;
}

ts_result_t ts_bear_request_signal(ts_bear9704_t *ctx)
{
    return start_query(ctx, TS_QUERY_SIGNAL, "GET constellationState {}\r");
}

ts_result_t ts_bear_request_imei(ts_bear9704_t *ctx)
{
    return start_query(ctx, TS_QUERY_IMEI, "GET hwInfo {}\r");
}

const char *ts_bear_imei(const ts_bear9704_t *ctx)
{
    return (ctx && ctx->imei[0]) ? ctx->imei : NULL;
}

int8_t ts_bear_signal_bars(const ts_bear9704_t *ctx)
{
    return ctx ? ctx->signal_bars : -1;
}

/* ---- Incoming lines ---- */

static void on_query_reply(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    uint32_t bars;
    if (ctx->query == TS_QUERY_SIGNAL && !strcmp(f->target, "constellationState")) {
        ctx->query = TS_QUERY_NONE;
        if (f->code == TS_JSPR_OK && ts_json_u32(f->json, "signal_bars", &bars) &&
            bars <= 5u) {
            ctx->signal_bars = (int8_t)bars;
            ts_emit(ctx, TS_EVENT_SIGNAL, (int32_t)bars);
        } else {
            ts_emit(ctx, TS_EVENT_ERROR, TS_ERR_PROTOCOL);
        }
    } else if (ctx->query == TS_QUERY_IMEI && !strcmp(f->target, "hwInfo")) {
        ctx->query = TS_QUERY_NONE;
        if (f->code == TS_JSPR_OK &&
            ts_json_string(f->json, "imei", ctx->imei, sizeof(ctx->imei))) {
            ts_emit(ctx, TS_EVENT_IMEI, 0);
        } else {
            ts_emit(ctx, TS_EVENT_ERROR, TS_ERR_PROTOCOL);
        }
    }
}

static void on_frame(ts_bear9704_t *ctx, const ts_jspr_frame_t *f)
{
    /* Code 299 is an unsolicited message about a transfer. */
    if (f->code == TS_JSPR_UNSOLICITED) {
#if TS_ENABLE_MT
        ts_mt_on_frame(ctx, f);
#endif
        if (ctx->mo_pending) ts_mo_on_frame(ctx, f);
        return;
    }
    if (ctx->state >= TS_API_QUERY && ctx->state <= TS_MODE_SET) {
        if (!ts_setup_on_frame(ctx, f)) ts_fail(ctx, TS_ERR_PROTOCOL);
    } else if (ctx->state == TS_MO_START || ctx->state == TS_MO_ACTIVE) {
        ts_mo_on_frame(ctx, f);
    } else if (ctx->state == TS_SHUTDOWN_WAIT &&
               !strcmp(f->target, "operationalState")) {
        if (f->code != TS_JSPR_OK) {
            ts_fail(ctx, TS_ERR_REJECTED);
            return;
        }
        power_off(ctx, TS_OFF);
    } else if (ctx->state == TS_READY && ctx->query) {
        on_query_reply(ctx, f);
    }
}

static void on_line(ts_bear9704_t *ctx)
{
    ts_jspr_frame_t frame;
    ctx->rx[ctx->rx_len] = '\0';
    if (ts_jspr_parse(ctx->rx, &frame)) on_frame(ctx, &frame);
    else ts_emit(ctx, TS_EVENT_ERROR, TS_ERR_PROTOCOL);
    ctx->rx_len = 0;
}

/* Returns false if the driver stopped (fault or power off). */
static bool read_serial(ts_bear9704_t *ctx)
{
    unsigned i;
    for (i = 0; i < TS_POLL_BYTE_BUDGET; ++i) {
        uint8_t byte;
        int16_t n = ctx->port.serial_read(ctx->port.user, &byte, 1);
        if (n < 0) { ts_fail(ctx, TS_ERR_IO); return false; }
        if (n == 0) break;
        if (byte == '\n' && ctx->rx_len == 0) continue;
        if (ctx->rx_discard) { /* Skip the rest of an oversized line. */
            if (byte == '\r') { ctx->rx_discard = false; ctx->rx_len = 0; }
            continue;
        }
        if (byte == '\r') {
            on_line(ctx);
            if (ctx->state == TS_POWERING_DOWN) return false;
        } else if (ctx->rx_len + 1u < sizeof(ctx->rx)) {
            ctx->rx[ctx->rx_len++] = (char)byte;
        } else {
            ctx->rx_discard = true;
            ctx->rx_len = 0;
            ts_emit(ctx, TS_EVENT_ERROR, TS_ERR_CAPACITY);
        }
    }
    return true;
}

static bool write_serial(ts_bear9704_t *ctx)
{
    int16_t n;
    if (!ts_tx_busy(ctx)) return true;
    n = ctx->port.serial_write(ctx->port.user,
                               (const uint8_t *)ctx->tx + ctx->tx_pos,
                               (size_t)(ctx->tx_len - ctx->tx_pos));
    if (n < 0 || (uint16_t)n > ctx->tx_len - ctx->tx_pos) {
        ts_fail(ctx, TS_ERR_IO);
        return false;
    }
    ctx->tx_pos = (uint16_t)(ctx->tx_pos + (uint16_t)n);
    return true;
}

/* Returns true once the modem has booted and the handshake has begun. */
static bool wait_for_boot(ts_bear9704_t *ctx)
{
    if (ctx->port.boot_ready(ctx->port.user)) {
        ctx->port.uart_enable(ctx->port.user, true);
        if (!ts_cmd(ctx, "GET apiVersion {}\r")) {
            ts_fail(ctx, TS_ERR_IO);
            return false;
        }
        ctx->state = TS_API_QUERY;
        return true;
    }
    if (ts_expired(ts_now(ctx), ctx->deadline)) ts_fail(ctx, TS_ERR_TIMEOUT);
    return false;
}

/* Every state that gets this far, except READY, is waiting for a modem reply.
 * READY waits only while an information request is outstanding. (ts_bear_poll()
 * returns earlier when the module is off, booting or powering down.) */
static bool waiting_for_reply(const ts_bear9704_t *ctx)
{
    return ctx->state != TS_READY || ctx->query != TS_QUERY_NONE;
}

static void check_timeouts(ts_bear9704_t *ctx)
{
    uint32_t now = ts_now(ctx);
    ts_mt_poll(ctx, now);
    if (!waiting_for_reply(ctx) || !ts_expired(now, ctx->deadline)) return;

    if (ctx->state == TS_MO_ACTIVE) {
        ts_mo_abort(ctx, TS_ERR_TIMEOUT);
    } else if (ctx->state == TS_READY) {
        ctx->query = TS_QUERY_NONE;
        ts_emit(ctx, TS_EVENT_ERROR, TS_ERR_TIMEOUT);
    } else {
        ts_fail(ctx, TS_ERR_TIMEOUT);
    }
}

void ts_bear_poll(ts_bear9704_t *ctx)
{
    if (!ctx || ctx->state == TS_OFF || ctx->state == TS_FAULT) return;
    if (ctx->state == TS_POWERING_DOWN) {
        wait_for_power_down(ctx);
        return;
    }
    if (ctx->state == TS_BOOTING && !wait_for_boot(ctx)) return;
    if (!write_serial(ctx) || !read_serial(ctx)) return;
    check_timeouts(ctx);
}
