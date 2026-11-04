/* SPDX-License-Identifier: MIT */
#include <msp430.h>

#include "app_payload.h"
#include "bear_checkout.h"
#include "ts_bear9704.h"
#include "ts_f169_console.h"
#include "ts_f169_port.h"

/*
 * MSP430F169 + BEAR Module 9704 reference firmware.
 *
 * Flash it, open the console (USART0, P3.4 TXD, 115200 8N1) and the first
 * modem cycle prints a full checkout: every step of power-up, handshake,
 * IMEI, signal, send and power-down with PASS / WARN / FAIL / SKIP and a
 * plain-language hint for each failure. Every later cycle prints a one-line
 * summary (plus any WARN/FAIL rows), so leaving it running is a soak test.
 *
 * The module is only powered while it has work to do:
 *
 *   wait REPORT_INTERVAL_MS -> checkout cycle (power on ... power off) -> wait
 *
 * Until TS_TOPIC_ID is set (after activation) no message is sent; the report
 * shows the IMEI and the activation steps instead.
 */

#ifndef REPORT_INTERVAL_MS
#define REPORT_INTERVAL_MS 600000UL /* One cycle every 10 minutes. */
#endif

/* ANSI colours in the console. Set to 0 for terminals that show "[32m". */
#ifndef TS_CONSOLE_COLOR
#define TS_CONSOLE_COLOR 1
#endif

/* Full report every cycle instead of only the first. */
#ifndef TS_CHECKOUT_VERBOSE_ALL
#define TS_CHECKOUT_VERBOSE_ALL 0
#endif

#if TS_ENABLE_CONSOLE && TS_F169_CONSOLE_RING < BC_LINE_CAP
#error TS_F169_CONSOLE_RING must hold one whole checkout line (BC_LINE_CAP)
#endif

#define TS_STRINGIFY_(x) #x
#define TS_STRINGIFY(x) TS_STRINGIFY_(x)

static bool console_write(void *user, const char *text)
{
    (void)user;
    return ts_f169_console_write(text);
}

static void on_event(void *user, ts_event_t event, int32_t detail)
{
    bear_checkout_on_event((bear_checkout_t *)user, event, detail);
}

/* Encodes the next report, or returns 0 when nothing should be sent. */
static size_t next_payload(const app_reading_t *reading, uint8_t *out, size_t capacity)
{
#if TS_TOPIC_ID < 64u
    (void)reading; (void)out; (void)capacity;
    return 0;
#else
    size_t length, bits;
    return app_payload_encode(reading, out, capacity, &length, &bits) ? length : 0u;
#endif
}

/* Lets the host emulator (tests/test_f169_port.c) return from main; 0 on target. */
#ifndef TS_FIRMWARE_STOP
#define TS_FIRMWARE_STOP() 0
#endif

int main(void)
{
    static ts_bear9704_t bear;
    static bear_checkout_t checkout;
    static uint8_t payload[TS_MAX_PAYLOAD];
    static app_reading_t reading = {0, 215};
    ts_platform_t port;
    bc_config_t config = {0};
    uint32_t last_start = 0;
    bool started = false;

    if (!ts_f169_init()) {
        /* XT2 did not start: no console or modem UART is possible. */
        for (;;) {
            if (TS_FIRMWARE_STOP()) return 1;
        }
    }
    ts_f169_console_init();
    port = ts_f169_platform();
    __enable_interrupt();

    config.write = console_write;
    config.board = "MSP430F169, XT2 8 MHz, watchdog "
                   TS_STRINGIFY(TS_F169_WATCHDOG);
    config.wiring = "ENABLE P1.0, BOOTED P1.1, UART1 TX P3.6 / RX P3.7";
    config.console = "USART0 TX P3.4, 115200 8N1; modem 230400 8N1";
    config.topic = TS_TOPIC_ID;
    config.interval_ms = REPORT_INTERVAL_MS;
    config.color = TS_CONSOLE_COLOR != 0;
    config.verbose_all = TS_CHECKOUT_VERBOSE_ALL != 0;

    (void)ts_bear_init(&bear, &port, on_event, &checkout);
    bear_checkout_init(&checkout, &bear, &config);

    for (;;) {
        uint32_t now;
        ts_bear_poll(&bear);
        now = port.millis(port.user);
        if (!bear_checkout_running(&checkout) &&
            (!started || now - last_start >= REPORT_INTERVAL_MS)) {
            size_t length = next_payload(&reading, payload, sizeof payload);
            (void)bear_checkout_begin(&checkout, now, length ? payload : NULL, length);
            started = true;
            last_start = now;
        }
        if (bear_checkout_poll(&checkout, now) && bear_checkout_sent(&checkout))
            ++reading.sequence; /* Gaps and repeats are visible to the cloud. */
        ts_f169_watchdog_kick();
        __bis_SR_register(LPM0_bits | GIE); /* Sleep until the next interrupt. */
        if (TS_FIRMWARE_STOP()) return 0;
    }
}
