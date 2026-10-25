/* SPDX-License-Identifier: MIT */
#ifndef TS_PLATFORM_H
#define TS_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * The hardware abstraction layer. To run the SDK on a new microcontroller,
 * fill in this struct (see ports/ for examples). Nothing else in the SDK
 * touches registers or pins.
 *
 * All functions are called from the main loop, never from an interrupt.
 * Reads and writes must not block: return what you could do right now.
 */
typedef struct {
    void *user; /* Passed back to every function below. */

    /* Return bytes read or written, 0 if none, negative on error (a lost
     * byte counts as an error). The UART runs 8N1 at the modem's baud rate
     * (230400 for the 9704). */
    int16_t (*serial_read)(void *user, uint8_t *dst, size_t capacity);
    int16_t (*serial_write)(void *user, const uint8_t *src, size_t length);

    /* Milliseconds since start. Wrapping around is fine. */
    uint32_t (*millis)(void *user);

    /* Power sequencing for the BEAR Module. power_start() returns true if the
     * module was switched on. uart_enable(false) must leave the MCU's TX pin
     * high impedance so the unpowered modem is not back-fed. boot_ready()
     * reports the module's "booted" signal.
     *
     * The module needs a complete boot and shutdown cycle. The driver turns
     * it on, waits for boot_ready() to be true before enabling the UART, and
     * after power_stop() waits for boot_ready() to be false before it will
     * start the module again. Your power_stop() must not remove main power
     * itself; do that only after the driver reports TS_EVENT_STOPPED. */
    bool (*power_start)(void *user);
    void (*power_stop)(void *user);
    void (*uart_enable)(void *user, bool enable);
    bool (*boot_ready)(void *user);
} ts_platform_t;

#endif
