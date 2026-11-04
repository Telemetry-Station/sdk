/* SPDX-License-Identifier: MIT */
#include "bear_checkout.h"

#include <string.h>

#include "ts_activation.h"

/* Cycle phases. Each one owns a check that is BC_RUN while it is active. */
enum {
    PH_IDLE = 0,
    PH_WAIT_OFF,  /* BOOTED must be low before ENABLE is raised. */
    PH_STARTING,  /* Driver boot and handshake. */
    PH_IMEI,
    PH_SIGNAL,
    PH_SEND,
    PH_SHUTDOWN,
    PH_SETTLE,    /* Driver faulted; waiting for it to finish powering down. */
    PH_DONE
};

/* Output sections, printed in this order. */
enum { OUT_HEADER = 0, OUT_ROWS, OUT_SUMMARY, OUT_STEPS, OUT_FINISHED };

/* What the buffered line advances once it has been written. */
enum { PEND_NONE = 0, PEND_CURSOR, PEND_ROW, PEND_MT, PEND_DROP, PEND_HEARTBEAT };

#define BC_HEARTBEAT_MS 15000ul
#define BC_RULE "------------------------------------------------------------\r\n"
#define BC_DOUBLE_RULE "============================================================\r\n"
#define BC_MAX_LINES_PER_POLL 8u
#define BC_LABEL_WIDTH 21u

static const char *const labels[BC_CHECK_COUNT] = {
    "Module off at reset", "Module boot", "Serial link", "SIM",
    "Radio active", "IMEI", "Satellite signal", "Message queued",
    "Delivery", "Power down"
};

const char *bear_checkout_label(bc_check_t check)
{
    return (unsigned)check < BC_CHECK_COUNT ? labels[check] : "?";
}

/* ---- Bounded line formatting (no stdio) ---- */

static void put(bear_checkout_t *ck, const char *text)
{
    /* Two bytes stay free for the CRLF added by end_line(). */
    while (*text && ck->len + 3u < BC_LINE_CAP) ck->line[ck->len++] = *text++;
    ck->line[ck->len] = '\0';
}

static void put_u32(bear_checkout_t *ck, uint32_t value, unsigned width, char fill)
{
    char digits[11];
    unsigned n = 0;
    do {
        digits[n++] = (char)('0' + (unsigned)(value % 10u));
        value /= 10u;
    } while (value);
    while (width > n) {
        char pad[2];
        pad[0] = fill;
        pad[1] = '\0';
        put(ck, pad);
        --width;
    }
    while (n) {
        char digit[2];
        digit[0] = digits[--n];
        digit[1] = '\0';
        put(ck, digit);
    }
}

static void put_padded(bear_checkout_t *ck, const char *text, unsigned width)
{
    unsigned n = (unsigned)strlen(text);
    put(ck, text);
    while (n++ < width) put(ck, " ");
}

/* "12.3" from milliseconds. */
static void put_seconds(bear_checkout_t *ck, uint32_t ms)
{
    put_u32(ck, ms / 1000u, 1u, ' ');
    put(ck, ".");
    put_u32(ck, (ms % 1000u) / 100u, 1u, '0');
}

static void put_color(bear_checkout_t *ck, const char *code)
{
    if (ck->cfg.color) put(ck, code);
}

static void begin_line(bear_checkout_t *ck)
{
    ck->len = 0;
    ck->line[0] = '\0';
}

static void end_line(bear_checkout_t *ck)
{
    ck->line[ck->len++] = '\r';
    ck->line[ck->len++] = '\n';
    ck->line[ck->len] = '\0';
}

static void put_stamp(bear_checkout_t *ck, uint32_t now)
{
    put_color(ck, "\x1b[2m");
    put(ck, "[");
    put_u32(ck, now / 1000u, 5u, ' ');
    put(ck, ".");
    put_u32(ck, now % 1000u, 3u, '0');
    put(ck, "]");
    put_color(ck, "\x1b[0m");
    put(ck, " ");
}

static void put_tag(bear_checkout_t *ck, uint8_t status)
{
    static const char *const tags[] = {"WAIT", "WAIT", "PASS", "WARN", "CHECK", "SKIP"};
    static const char *const colors[] = {
        "\x1b[36m", "\x1b[36m", "\x1b[1;32m", "\x1b[1;33m", "\x1b[1;31m", "\x1b[2m"
    };
    put_color(ck, colors[status]);
    put(ck, tags[status]);
    put_color(ck, "\x1b[0m");
    put(ck, status == BC_FAIL ? "  " : "   "); /* Tags are padded to five columns. */
}

static void put_verdict(bear_checkout_t *ck, bc_verdict_t verdict)
{
    static const uint8_t as_status[] = {BC_PASS, BC_WARN, BC_FAIL};
    put_tag(ck, as_status[verdict]);
}

const char *bear_checkout_reason(int32_t reason)
{
    switch (reason) {
    case TS_ERR_TIMEOUT:   return "timed out";
    case TS_ERR_IO:        return "serial I/O error or BOOTED dropped";
    case TS_ERR_PROTOCOL:  return "unexpected reply from modem";
    case TS_ERR_REJECTED:  return "rejected by modem";
    case TS_ERR_CAPACITY:  return "message too large";
    case TS_ERR_BUSY:      return "driver busy";
    case TS_ERR_NOT_READY: return "modem not ready";
    case TS_ERR_ARGUMENT:  return "invalid argument";
    default:               return "error";
    }
}

/* ---- Check bookkeeping ---- */

static uint32_t clock_now(const bear_checkout_t *ck)
{
    return ck->bear->port.millis(ck->bear->port.user);
}

static void run_check(bear_checkout_t *ck, uint8_t id, uint32_t now)
{
    ck->status[id] = BC_RUN;
    ck->mark[id] = now;
}

static void finish(bear_checkout_t *ck, uint8_t id, uint8_t status,
                   int32_t value, int32_t reason)
{
    ck->status[id] = status;
    ck->value[id] = value;
    ck->reason[id] = reason;
}

static void pass_elapsed(bear_checkout_t *ck, uint8_t id, uint32_t now)
{
    if (ck->status[id] == BC_RUN)
        finish(ck, id, BC_PASS, (int32_t)(now - ck->mark[id]), 0);
}

/* Skips every check not yet started, from `first` up to (not including) `last`. */
static void skip_range(bear_checkout_t *ck, uint8_t first, uint8_t last)
{
    uint8_t i;
    for (i = first; i < last; ++i)
        if (ck->status[i] == BC_WAIT) finish(ck, i, BC_SKIP, 0, 0);
}

/* The driver's last reported error. Every driver failure reports one, so the
 * fallback only shows when other code stopped or used the module. */
static int32_t error_or(const bear_checkout_t *ck, int32_t fallback)
{
    return ck->last_error ? ck->last_error : fallback;
}

static void stop_cycle(bear_checkout_t *ck)
{
    skip_range(ck, 0, BC_CHECK_COUNT);
    ck->phase = PH_DONE;
}

/* The driver gave up during start-up or a transfer. Fail the running check,
 * skip the rest, and wait for the driver to finish powering the module down. */
static void abort_cycle(bear_checkout_t *ck, uint32_t now)
{
    uint8_t i;
    int32_t reason = error_or(ck, TS_ERR_BUSY);
    /* While booting the driver only gives up on a BOOTED timeout. Any other
     * fault means BOOTED rose and the handshake failed in the same poll. */
    if (ck->status[BC_BOOT] == BC_RUN && reason != TS_ERR_TIMEOUT) {
        pass_elapsed(ck, BC_BOOT, now);
        run_check(ck, BC_LINK, now);
    }
    for (i = BC_BOOT; i < BC_SHUTDOWN; ++i)
        if (ck->status[i] == BC_RUN) finish(ck, i, BC_FAIL, 0, reason);
    skip_range(ck, BC_BOOT, BC_SHUTDOWN);
    run_check(ck, BC_SHUTDOWN, now);
    ck->phase = PH_SETTLE;
}

static void enter(bear_checkout_t *ck, uint8_t phase, uint8_t check, uint32_t now)
{
    ck->phase = phase;
    ck->asked = false;
    ck->last_error = 0;
    run_check(ck, check, now);
}

static void enter_send(bear_checkout_t *ck, uint32_t now)
{
    if (ck->cfg.topic < TS_MIN_TOPIC_ID || !ck->payload || !ck->payload_len) {
        finish(ck, BC_SEND, BC_SKIP, 1, 0); /* value 1: not configured. */
        finish(ck, BC_DELIVERY, BC_SKIP, 1, 0);
        enter(ck, PH_SHUTDOWN, BC_SHUTDOWN, now);
        return;
    }
    enter(ck, PH_SEND, BC_SEND, now);
}

static bool booted(const bear_checkout_t *ck)
{
    return ck->bear->port.boot_ready(ck->bear->port.user);
}

static void start_module(bear_checkout_t *ck, uint32_t now)
{
    ts_result_t r = ts_bear_start(ck->bear);
    if (r != TS_OK) {
        finish(ck, BC_BOOT, BC_FAIL, 0, r);
        stop_cycle(ck);
        return;
    }
    run_check(ck, BC_BOOT, now);
    ck->phase = PH_STARTING;
}

static void take_mt(bear_checkout_t *ck)
{
#if TS_ENABLE_MT
    /* Bench policy: report and discard incoming data. A product hands it to
     * its own command handler instead. */
    uint8_t data[TS_MAX_PAYLOAD];
    size_t length;
    uint16_t topic;
    if (ts_bear_take_mt(ck->bear, data, sizeof data, &length, &topic) == TS_OK) {
        ck->mt_length = (uint16_t)length;
        ck->mt_topic = topic;
        ck->mt_note = true;
    }
#else
    (void)ck;
#endif
}

/* One information request (IMEI or signal) as a check. */
static void query_step(bear_checkout_t *ck, uint8_t check, ts_query_t query,
                       bool answered, uint32_t now)
{
    ts_result_t r;
    if (answered) {
        if (check == BC_SIGNAL)
            finish(ck, check, ck->bars > 0 ? BC_PASS : BC_WARN, ck->bars, 0);
        else
            pass_elapsed(ck, check, now);
    } else if (ck->bear->query == query) {
        ck->asked = true;
        return;
    } else if (ck->asked) {
        finish(ck, check, BC_FAIL, 0, error_or(ck, TS_ERR_PROTOCOL));
    } else {
        /* BUSY/NOT_READY: the driver is finishing other work (another query,
         * a command still being written). Its own deadlines bound the wait. */
        r = query == TS_QUERY_IMEI ? ts_bear_request_imei(ck->bear)
                                   : ts_bear_request_signal(ck->bear);
        if (r == TS_OK) ck->asked = true;
        return;
    }
    if (check == BC_IMEI) enter(ck, PH_SIGNAL, BC_SIGNAL, now);
    else enter_send(ck, now);
}

static void send_step(bear_checkout_t *ck, uint32_t now)
{
    if (!ck->sent) {
        ts_result_t r = ts_bear_send(ck->bear, ck->cfg.topic, ck->payload, ck->payload_len);
        /* Topic and payload were checked up front, so only NOT_READY (driver
         * busy for a moment) is expected; anything else fails the send. */
        if (r == TS_OK) ck->sent = true;
        else if (r != TS_ERR_NOT_READY) finish(ck, BC_SEND, BC_FAIL, 0, r);
    }
    if (ck->status[BC_SEND] != BC_RUN && ck->status[BC_DELIVERY] != BC_RUN) {
        skip_range(ck, BC_DELIVERY, BC_SHUTDOWN); /* Not started if the send failed. */
        enter(ck, PH_SHUTDOWN, BC_SHUTDOWN, now);
    }
}

/* Off, powering down or faulted: the module is no longer running. */
static bool module_down(ts_state_t state)
{
    return state == TS_OFF || state >= TS_POWERING_DOWN;
}

static void shutdown_step(bear_checkout_t *ck, ts_state_t state, uint32_t now)
{
    if (state == TS_OFF) {
        pass_elapsed(ck, BC_SHUTDOWN, now);
        ck->phase = PH_DONE;
    } else if (state == TS_FAULT || state == TS_FAULT_HOLD) {
        finish(ck, BC_SHUTDOWN, BC_FAIL,
               state == TS_FAULT_HOLD ? 2 : (booted(ck) ? 1 : 0),
               error_or(ck, TS_ERR_TIMEOUT));
        ck->phase = PH_DONE;
    } else if (!ck->asked && ts_bear_shutdown(ck->bear) == TS_OK) {
        /* BUSY while a received message, a query or other work is in progress. */
        ck->asked = true;
    }
}

static void settle_step(bear_checkout_t *ck, ts_state_t state, uint32_t now)
{
    if (state == TS_POWERING_DOWN) return;
    if (state == TS_OFF) /* Shut down cleanly, by other code. */
        pass_elapsed(ck, BC_SHUTDOWN, now);
    else if (state == TS_FAULT && !booted(ck))
        finish(ck, BC_SHUTDOWN, BC_PASS, -1, ck->last_error); /* Forced off. */
    else
        finish(ck, BC_SHUTDOWN, BC_FAIL, state == TS_FAULT_HOLD ? 2 : 1,
               ck->last_error);
    ck->phase = PH_DONE;
}

static void startup_step(bear_checkout_t *ck, ts_state_t state, uint32_t now)
{
    /* The handshake is strictly ordered, so reaching a state proves every
     * earlier step worked, even if several happened in one poll. */
    if (state >= TS_API_QUERY) {
        if (ck->status[BC_BOOT] == BC_RUN) {
            pass_elapsed(ck, BC_BOOT, now);
            run_check(ck, BC_LINK, now);
        }
    }
    if (state >= TS_SIM_QUERY && ck->status[BC_LINK] == BC_RUN) {
        pass_elapsed(ck, BC_LINK, now);
        run_check(ck, BC_SIM, now);
    }
    if (state >= TS_MODE_QUERY && ck->status[BC_SIM] == BC_RUN) {
        pass_elapsed(ck, BC_SIM, now);
        run_check(ck, BC_ACTIVE, now);
    }
    if (state == TS_READY) {
        pass_elapsed(ck, BC_ACTIVE, now);
        ck->phase = PH_IMEI;
        ck->asked = false;
        run_check(ck, BC_IMEI, now);
    }
}

static void step(bear_checkout_t *ck, uint32_t now)
{
    ts_state_t state = ts_bear_state(ck->bear);

    if (ck->phase >= PH_STARTING && ck->phase <= PH_SHUTDOWN) take_mt(ck);
    if (ck->phase >= PH_STARTING && ck->phase <= PH_SEND && module_down(state))
        abort_cycle(ck, now);

    switch (ck->phase) {
    case PH_WAIT_OFF:
        if (!booted(ck)) {
            /* asked: BOOTED was seen high, so report how long it took to fall. */
            finish(ck, BC_MODULE_OFF, BC_PASS,
                   ck->asked ? (int32_t)(now - ck->mark[BC_MODULE_OFF]) : 0, 0);
            start_module(ck, now);
            break;
        }
        ck->asked = true;
        if (now - ck->mark[BC_MODULE_OFF] >= TS_BOOT_TIMEOUT_MS) {
            finish(ck, BC_MODULE_OFF, BC_FAIL, 0, TS_ERR_TIMEOUT);
            stop_cycle(ck);
        }
        break;
    case PH_STARTING:
        startup_step(ck, state, now);
        break;
    case PH_IMEI:
        query_step(ck, BC_IMEI, TS_QUERY_IMEI, ck->got_reply, now);
        break;
    case PH_SIGNAL:
        query_step(ck, BC_SIGNAL, TS_QUERY_SIGNAL, ck->bars >= 0, now);
        break;
    case PH_SEND:
        send_step(ck, now);
        break;
    case PH_SHUTDOWN:
        shutdown_step(ck, state, now);
        break;
    case PH_SETTLE:
        settle_step(ck, state, now);
        break;
    default:
        break;
    }
}

/* ---- Report text ---- */

static bool is_final(uint8_t status)
{
    return status >= BC_PASS;
}

static void put_ms(bear_checkout_t *ck, const char *before, int32_t ms, const char *after)
{
    put(ck, before);
    put_u32(ck, (uint32_t)ms, 1u, ' ');
    put(ck, " ms");
    put(ck, after);
}

static void put_detail(bear_checkout_t *ck, uint8_t id)
{
    uint8_t status = ck->status[id];
    int32_t value = ck->value[id];
    int32_t reason = ck->reason[id];

    if (status == BC_SKIP) {
        if (value == 1 && id == BC_SEND)
            put(ck, ck->cfg.topic < TS_MIN_TOPIC_ID ? "TS_TOPIC_ID not set"
                                                    : "no payload to send");
        else if (value == 1) put(ck, "nothing sent");
        else put(ck, "depends on an earlier check");
        return;
    }
    if (status == BC_FAIL) {
        switch (id) {
        case BC_MODULE_OFF:
            put(ck, reason == TS_ERR_TIMEOUT ? "BOOTED stuck high; check BOOTED wiring"
                                             : "driver still holding the module");
            return;
        case BC_BOOT:
            put(ck, reason == TS_ERR_TIMEOUT ? "BOOTED never rose; check supply and ENABLE"
                                             : bear_checkout_reason(reason));
            return;
        case BC_LINK:
            put(ck, reason == TS_ERR_TIMEOUT ? "no reply; check TX/RX crossover and baud"
                                             : bear_checkout_reason(reason));
            return;
        case BC_DELIVERY:
            if (reason == TS_ERR_TIMEOUT) put(ck, "not confirmed in time; outcome unknown");
            else if (reason == TS_ERR_REJECTED) put(ck, "network did not confirm");
            else put(ck, bear_checkout_reason(reason));
            return;
        case BC_SHUTDOWN:
            if (value == 2) put(ck, "BOOTED low, ENABLE held; power-cycle board");
            else if (value == 1) put(ck, "BOOTED still high; module did not stop");
            else put(ck, reason == TS_ERR_REJECTED ? "modem refused shutdown"
                                                   : bear_checkout_reason(reason));
            return;
        default:
            put(ck, bear_checkout_reason(reason));
            return;
        }
    }
    switch (id) { /* PASS or WARN */
    case BC_MODULE_OFF:
        if (value > 0) put_ms(ck, "BOOTED fell after ", value, "");
        else put(ck, "BOOTED low");
        break;
    case BC_BOOT:   put_ms(ck, "BOOTED high after ", value, ""); break;
    case BC_LINK:   put_ms(ck, "modem replied in ", value, ""); break;
    case BC_SIM:    put(ck, "internal SIM selected"); break;
    case BC_ACTIVE: put(ck, "operational state active"); break;
    case BC_IMEI:   put(ck, ts_bear_imei(ck->bear)); break;
    case BC_SIGNAL:
        put_u32(ck, (uint32_t)value, 1u, ' ');
        put(ck, value > 0 ? "/5 bars" : "/5 bars; antenna needs open sky");
        break;
    case BC_SEND:
        put(ck, "accepted as message ");
        put_u32(ck, (uint32_t)value, 1u, ' ');
        put(ck, ", ");
        put_u32(ck, ck->payload_len, 1u, ' ');
        put(ck, " bytes");
        break;
    case BC_DELIVERY:
        put(ck, "confirmed by Iridium in ");
        put_seconds(ck, (uint32_t)value);
        put(ck, " s");
        break;
    default: /* BC_SHUTDOWN */
        if (value < 0) put(ck, "module off after fault");
        else put_ms(ck, "BOOTED low after ", value, "; safe to cut power");
        break;
    }
}

static const char *const waiting_text[BC_CHECK_COUNT] = {
    "waiting for BOOTED to fall", "waiting for BOOTED to rise",
    "waiting for the modem", "waiting for the modem", "waiting for the modem",
    "waiting for the modem", "waiting for the modem",
    "waiting for the modem to accept", "waiting for satellite confirmation",
    "waiting for BOOTED to fall"
};

static void build_row(bear_checkout_t *ck, uint8_t id, uint32_t now)
{
    put_stamp(ck, now);
    put_tag(ck, ck->status[id]);
    put_padded(ck, labels[id], BC_LABEL_WIDTH);
    if (ck->status[id] == BC_RUN) {
        put(ck, waiting_text[id]);
        put(ck, " (");
        put_u32(ck, (now - ck->mark[id]) / 1000u, 1u, ' ');
        put(ck, " s)");
    } else {
        put_detail(ck, id);
    }
    end_line(ck);
}

static void count(const bear_checkout_t *ck, unsigned counts[6])
{
    unsigned i;
    memset(counts, 0, 6u * sizeof counts[0]);
    for (i = 0; i < BC_CHECK_COUNT; ++i) counts[ck->status[i]]++;
}

static void put_counts(bear_checkout_t *ck)
{
    unsigned counts[6];
    count(ck, counts);
    put_u32(ck, counts[BC_PASS], 1u, ' ');
    put(ck, " pass  ");
    put_u32(ck, counts[BC_WARN], 1u, ' ');
    put(ck, " warn  ");
    put_u32(ck, counts[BC_FAIL], 1u, ' ');
    put(ck, " check  ");
    put_u32(ck, counts[BC_SKIP], 1u, ' ');
    put(ck, " skip");
}

static void put_tally(bear_checkout_t *ck)
{
    put(ck, "(");
    put_u32(ck, ck->cycles_passed, 1u, ' ');
    put(ck, "/");
    put_u32(ck, ck->cycle, 1u, ' ');
    put(ck, " cycles passed)");
}

/* Header, summary and next-steps lines. False when the section is over. */
static bool build_header(bear_checkout_t *ck)
{
    switch (ck->cursor) {
    case 0: put(ck, "\r\n"); put(ck, BC_DOUBLE_RULE); return true;
    case 1:
        put_color(ck, "\x1b[1m");
        put(ck, " Telemetry Station SDK v" TS_SDK_VERSION_STRING "  |  BEAR Module 9704 checkout");
        put_color(ck, "\x1b[0m");
        end_line(ck);
        return true;
    case 2: put(ck, BC_DOUBLE_RULE); return true;
    case 3: if (!ck->cfg.board) return true;
        put(ck, " Board    "); put(ck, ck->cfg.board); end_line(ck); return true;
    case 4: if (!ck->cfg.wiring) return true;
        put(ck, " Wiring   "); put(ck, ck->cfg.wiring); end_line(ck); return true;
    case 5: if (!ck->cfg.console) return true;
        put(ck, " Console  "); put(ck, ck->cfg.console); end_line(ck); return true;
    case 6:
        put(ck, " Topic    ");
        if (ck->cfg.topic >= TS_MIN_TOPIC_ID) put_u32(ck, ck->cfg.topic, 1u, ' ');
        else put(ck, "not set; send test skipped");
        end_line(ck);
        return true;
    case 7:
        put(ck, " Options  receive ");
        put(ck, TS_ENABLE_MT ? "on" : "off");
        put(ck, ", payload max ");
        put_u32(ck, TS_MAX_PAYLOAD, 1u, ' ');
        put(ck, " B, cycle every ");
        put_u32(ck, ck->cfg.interval_ms / 1000u, 1u, ' ');
        put(ck, " s");
        end_line(ck);
        return true;
    case 8: put(ck, BC_RULE); return true;
    default: return false;
    }
}

static bool build_summary(bear_checkout_t *ck, uint32_t now)
{
    bc_verdict_t verdict = bear_checkout_verdict(ck);
    if (!ck->verbose) {
        if (ck->cursor) return false;
        put_stamp(ck, now);
        put(ck, "Cycle ");
        put_u32(ck, ck->cycle, 1u, ' ');
        put(ck, "  ");
        put_verdict(ck, verdict);
        put_counts(ck);
        put(ck, "  ");
        put_seconds(ck, now - ck->cycle_start);
        put(ck, " s ");
        put_tally(ck);
        end_line(ck);
        return true;
    }
    switch (ck->cursor) {
    case 0: put(ck, BC_RULE); return true;
    case 1:
        put(ck, " RESULT   ");
        put_verdict(ck, verdict);
        put_counts(ck);
        put(ck, "   ");
        put_seconds(ck, now - ck->cycle_start);
        put(ck, " s");
        end_line(ck);
        return true;
    case 2: put(ck, BC_RULE); return true;
    case 3:
        put(ck, " Next cycle in ");
        put_u32(ck, ck->cfg.interval_ms / 1000u, 1u, ' ');
        put(ck, " s ");
        put_tally(ck);
        end_line(ck);
        return true;
    default: return false;
    }
}

static bool build_steps(bear_checkout_t *ck)
{
    switch (ck->cursor) {
    case 0: put(ck, "\r\n Next steps to send data:\r\n"); return true;
    case 1: put(ck, "  1. Payload Optimizer  " TS_OPTIMIZER_URL "\r\n"); return true;
    case 2: put(ck, "  2. Plan calculator    " TS_CALCULATOR_URL "\r\n"); return true;
    case 3:
        put(ck, "  3. Activate IMEI ");
        put(ck, ts_bear_imei(ck->bear));
        put(ck, " at " TS_ACTIVATE_URL);
        end_line(ck);
        return true;
    case 4: put(ck, "  4. Set TS_TOPIC_ID to your topic, rebuild and flash.\r\n\r\n"); return true;
    default: return false;
    }
}

static bool build_rows(bear_checkout_t *ck, uint32_t now)
{
    if (ck->mt_note) {
        put_stamp(ck, now);
        put_tag(ck, BC_PASS);
        put_padded(ck, "Received message", BC_LABEL_WIDTH);
        put_u32(ck, ck->mt_length, 1u, ' ');
        put(ck, " bytes on topic ");
        put_u32(ck, ck->mt_topic, 1u, ' ');
        put(ck, " (discarded)");
        end_line(ck);
        ck->pending = PEND_MT;
        return true;
    }
    if (ck->mt_dropped) {
        put_stamp(ck, now);
        put_tag(ck, BC_WARN);
        put_padded(ck, "Received message", BC_LABEL_WIDTH);
        put(ck, "incomplete or corrupt; dropped");
        end_line(ck);
        ck->pending = PEND_DROP;
        return true;
    }
    /* The compact report prints only problems, then its summary line. */
    while (ck->row < BC_CHECK_COUNT && !ck->verbose &&
           (ck->status[ck->row] == BC_PASS || ck->status[ck->row] == BC_SKIP))
        ++ck->row;
    if (ck->row >= BC_CHECK_COUNT) return false;
    if (is_final(ck->status[ck->row])) {
        build_row(ck, ck->row, now);
        ck->pending = PEND_ROW;
        return true;
    }
    /* Not final, so running: each phase starts its check as the last one ends. */
    if (ck->verbose && now - ck->heartbeat_at >= BC_HEARTBEAT_MS) {
        build_row(ck, ck->row, now);
        ck->pending = PEND_HEARTBEAT;
        return true;
    }
    return false;
}

/* Fills ck->line with the next line to print. False if there is none yet. */
static bool build_any(bear_checkout_t *ck, uint32_t now)
{
    for (;;) {
        begin_line(ck);
        ck->pending = PEND_CURSOR;
        switch (ck->out) {
        case OUT_HEADER:
            if (build_header(ck)) return true;
            ck->out = OUT_ROWS;
            ck->cursor = 0;
            break;
        case OUT_ROWS:
            if (build_rows(ck, now)) return true;
            /* Every check is final once DONE, so all rows have been printed. */
            if (ck->phase != PH_DONE) return false;
            ck->out = OUT_SUMMARY;
            if (bear_checkout_verdict(ck) != BC_CYCLE_FAIL) ck->cycles_passed++;
            break;
        case OUT_SUMMARY:
            if (build_summary(ck, now)) return true;
            ck->out = (ck->verbose && ck->cfg.topic < TS_MIN_TOPIC_ID &&
                       ck->status[BC_IMEI] == BC_PASS) ? OUT_STEPS : OUT_FINISHED;
            ck->cursor = 0;
            break;
        case OUT_STEPS:
            if (build_steps(ck)) return true;
            ck->out = OUT_FINISHED;
            break;
        default:
            return false;
        }
    }
}

static bool build(bear_checkout_t *ck, uint32_t now)
{
    if (build_any(ck, now)) return true;
    ck->pending = PEND_NONE;
    return false;
}

static void commit(bear_checkout_t *ck, uint32_t now)
{
    switch (ck->pending) {
    case PEND_ROW: ck->row++; ck->heartbeat_at = now; break;
    case PEND_MT: ck->mt_note = false; break;
    case PEND_DROP: ck->mt_dropped = 0; break;
    case PEND_HEARTBEAT: ck->heartbeat_at = now; break;
    default: ck->cursor++; break;
    }
    ck->pending = PEND_NONE;
    ck->len = 0;
}

static void render(bear_checkout_t *ck, uint32_t now)
{
    unsigned i;
    for (i = 0; i < BC_MAX_LINES_PER_POLL; ++i) {
        if (ck->pending == PEND_NONE && !build(ck, now)) return;
        /* An empty line (an omitted header field) needs no write. */
        if (ck->len && !ck->cfg.write(ck->cfg.write_user, ck->line)) return;
        commit(ck, now);
    }
}

/* ---- Public API ---- */

void bear_checkout_init(bear_checkout_t *ck, ts_bear9704_t *bear,
                        const bc_config_t *config)
{
    memset(ck, 0, sizeof *ck);
    ck->cfg = *config;
    ck->bear = bear;
    ck->out = OUT_FINISHED;
}

bool bear_checkout_begin(bear_checkout_t *ck, uint32_t now,
                         const uint8_t *payload, size_t length)
{
    ts_state_t state = ts_bear_state(ck->bear);
    if (ck->running) return false;
    memset(ck->status, 0, sizeof ck->status);
    memset(ck->value, 0, sizeof ck->value);
    memset(ck->reason, 0, sizeof ck->reason);
    ck->payload = payload;
    ck->payload_len = (uint16_t)(length <= TS_MAX_PAYLOAD ? length : 0u);
    ck->cycle_start = ck->heartbeat_at = now;
    ck->cycle++;
    ck->verbose = ck->cycle == 1u || ck->cfg.verbose_all;
    ck->out = ck->verbose ? OUT_HEADER : OUT_ROWS;
    ck->row = ck->cursor = 0;
    ck->pending = PEND_NONE;
    ck->len = 0;
    ck->running = true;
    ck->sent = ck->got_reply = ck->mt_note = false;
    ck->mt_dropped = 0;
    ck->bars = -1;
    enter(ck, PH_WAIT_OFF, BC_MODULE_OFF, now);
    if (state != TS_OFF && state != TS_FAULT) {
        /* FAULT_HOLD or a cycle the application started itself. */
        finish(ck, BC_MODULE_OFF, BC_FAIL, 0, TS_ERR_BUSY);
        stop_cycle(ck);
    }
    return true;
}

void bear_checkout_on_event(bear_checkout_t *ck, ts_event_t event, int32_t detail)
{
    uint32_t now;
    if (!ck->running) return;
    now = clock_now(ck);
    switch (event) {
    case TS_EVENT_IMEI:
        ck->got_reply = true;
        break;
    case TS_EVENT_SIGNAL:
        ck->bars = (int8_t)detail;
        break;
    case TS_EVENT_MO_ACCEPTED:
        if (ck->status[BC_SEND] == BC_RUN) {
            finish(ck, BC_SEND, BC_PASS, detail, 0);
            run_check(ck, BC_DELIVERY, now);
        }
        break;
    case TS_EVENT_MO_DELIVERED:
        pass_elapsed(ck, BC_DELIVERY, now);
        break;
    case TS_EVENT_MO_FAILED:
        if (ck->status[BC_SEND] == BC_RUN) {
            finish(ck, BC_SEND, BC_FAIL, 0, detail);
            finish(ck, BC_DELIVERY, BC_SKIP, 0, 0);
        } else if (ck->status[BC_DELIVERY] == BC_RUN) {
            finish(ck, BC_DELIVERY, BC_FAIL, (int32_t)(now - ck->mark[BC_DELIVERY]), detail);
        }
        break;
    case TS_EVENT_MT_DROPPED:
        ck->mt_dropped = 1u; /* Reported once per cycle. */
        break;
    case TS_EVENT_ERROR:
        ck->last_error = detail;
        break;
    default:
        break;
    }
}

bool bear_checkout_poll(bear_checkout_t *ck, uint32_t now)
{
    if (!ck->running) return false;
    step(ck, now);
    render(ck, now);
    if (ck->phase == PH_DONE && ck->out == OUT_FINISHED) {
        ck->running = false;
        ck->phase = PH_IDLE;
        return true;
    }
    return false;
}

bool bear_checkout_running(const bear_checkout_t *ck)
{
    return ck->running;
}

bc_status_t bear_checkout_status(const bear_checkout_t *ck, bc_check_t check)
{
    return (unsigned)check < BC_CHECK_COUNT ? (bc_status_t)ck->status[check] : BC_SKIP;
}

bc_verdict_t bear_checkout_verdict(const bear_checkout_t *ck)
{
    unsigned counts[6];
    count(ck, counts);
    if (counts[BC_FAIL] || counts[BC_WAIT] || counts[BC_RUN]) return BC_CYCLE_FAIL;
    return counts[BC_WARN] ? BC_CYCLE_WARN : BC_CYCLE_PASS;
}

bool bear_checkout_sent(const bear_checkout_t *ck)
{
    return ck->status[BC_SEND] == BC_PASS;
}
