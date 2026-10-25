/* SPDX-License-Identifier: MIT */
#include <msp430.h>
#include <stdio.h>

#include "app_payload.h"
#include "ts_activation.h"
#include "ts_bear9704.h"
#include "ts_f169_console.h"
#include "ts_f169_port.h"

/* Set your topic ID from Telemetry Station (TS_MIN_TOPIC_ID or higher). Until
 * then the firmware only starts the module, shows its IMEI, and shuts it down. */
#ifndef REPORT_INTERVAL_MS
#define REPORT_INTERVAL_MS 600000UL /* One message every 10 minutes. */
#endif

/*
 * The module is the biggest power consumer on the board, so it is only on
 * while there is work to do:
 *
 *   SLEEPING --(interval elapsed)--> WORKING --(sent or nothing to send)-->
 *   FINISHING --(module powered down)--> SLEEPING
 *
 * After the shutdown handshake completes, a real product would also switch off
 * the module's supply to save power.
 */
typedef enum { APP_SLEEPING, APP_WORKING, APP_FINISHING } app_state_t;

typedef struct {
    app_state_t state;
    uint32_t last_start;
    bool started_once;
    bool message_sent;
    bool message_done; /* Delivered, failed, or nothing to send. */
    app_reading_t reading;
} app_t;

static app_t app = {APP_SLEEPING, 0, false, false, false, {0, 215}};

static void on_event(void *user, ts_event_t event, int32_t detail)
{
    char text[256];
    ts_bear9704_t *bear = (ts_bear9704_t *)user;
    (void)detail;

    switch (event) {
    case TS_EVENT_IMEI:
        /* Step one of activation: show the IMEI and where to enter it. */
        if (ts_activation_text(ts_bear_imei(bear), 4u, text, sizeof text))
            ts_f169_console_print(text);
        break;
    case TS_EVENT_MO_DELIVERED:
        ts_f169_console_print("Message delivered\r\n");
        app.message_done = true;
        break;
    case TS_EVENT_MO_FAILED:
        ts_f169_console_print("Message failed\r\n");
        app.message_done = true;
        break;
    default:
        break;
    }
}

static void try_send(ts_bear9704_t *bear)
{
    uint8_t bytes[TS_MAX_PAYLOAD];
    size_t length, bits;

    if (TS_TOPIC_ID < TS_MIN_TOPIC_ID) {
        ts_f169_console_print("Set TS_TOPIC_ID after activating, then rebuild.\r\n");
        app.message_done = true;
        return;
    }
    if (!app_payload_encode(&app.reading, bytes, sizeof bytes, &length, &bits)) {
        app.message_done = true;
        return;
    }
    /* Fails with TS_ERR_NOT_READY while the IMEI query is still answering;
     * the next pass of the main loop tries again. */
    if (ts_bear_send(bear, TS_TOPIC_ID, bytes, length) == TS_OK) {
        app.message_sent = true;
        ++app.reading.sequence;
    }
}

static void run_app(ts_bear9704_t *bear, uint32_t now)
{
    ts_state_t state = ts_bear_state(bear);

    switch (app.state) {
    case APP_SLEEPING:
        if (!app.started_once || now - app.last_start >= REPORT_INTERVAL_MS) {
            if (ts_bear_start(bear) == TS_OK) {
                app.state = APP_WORKING;
                app.started_once = true;
                app.last_start = now;
                app.message_sent = app.message_done = false;
            }
        }
        break;
    case APP_WORKING:
        if (state == TS_FAULT) {
            app.state = APP_SLEEPING; /* The driver already powered down. Try next interval. */
        } else if (app.message_done) {
            if (ts_bear_shutdown(bear) == TS_OK) app.state = APP_FINISHING;
        } else if (ts_bear_is_ready(bear) && !app.message_sent) {
            try_send(bear);
        }
        break;
    case APP_FINISHING:
        if (state == TS_OFF || state == TS_FAULT) app.state = APP_SLEEPING;
        break;
    }
}

int main(void)
{
    static ts_bear9704_t bear;
    ts_platform_t port;

    ts_f169_init();
    ts_f169_console_init();
    port = ts_f169_platform();
    __enable_interrupt();

    ts_bear_init(&bear, &port, on_event, &bear);

    for (;;) {
        ts_bear_poll(&bear);
        run_app(&bear, port.millis(port.user));
        ts_f169_watchdog_kick();
        __bis_SR_register(LPM0_bits | GIE); /* Sleep until the next interrupt. */
    }
}
