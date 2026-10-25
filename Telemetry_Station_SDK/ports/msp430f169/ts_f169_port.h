/* SPDX-License-Identifier: MIT */
#ifndef TS_F169_PORT_H
#define TS_F169_PORT_H

#include "ts_platform.h"

/*
 * Platform adapter for the TI MSP430F169.
 *
 * Hardware used:
 *   - XT2 crystal at 8 MHz as MCLK and SMCLK. The 9704 UART runs at 230400
 *     baud, which the internal DCO is not accurate enough to hold.
 *   - USART1 as the modem UART: P3.6 = TXD1, P3.7 = RXD1.
 *   - Timer_A for a 1 ms tick.
 *   - Two GPIO pins for BEAR Module power control (below).
 *
 * The pin choices below are the test-bench wiring and can be changed with
 * -D flags. Confirm them against the BEAR Module 9704 documentation, and the
 * 3.3 V logic levels of both boards, before applying power.
 */

/* Output that switches the BEAR Module on (high = on). */
#ifndef TS_F169_ENABLE_OUT
#define TS_F169_ENABLE_OUT P1OUT
#define TS_F169_ENABLE_DIR P1DIR
#define TS_F169_ENABLE_BIT BIT0
#endif

/* Input that reads the BEAR Module's "booted" signal (high = booted). */
#ifndef TS_F169_BOOTED_IN
#define TS_F169_BOOTED_IN  P1IN
#define TS_F169_BOOTED_DIR P1DIR
#define TS_F169_BOOTED_BIT BIT1
#endif

/* Size of the UART buffers filled by interrupts. Power of two, 256 or less. */
#ifndef TS_F169_RING_SIZE
#define TS_F169_RING_SIZE 128u
#endif

/* Hardware watchdog. Off by default so the bench port works on any board.
 * Turn it on before deployment (-DTS_F169_WATCHDOG=1). It resets the MCU if
 * ts_f169_watchdog_kick() is not called for about one second, and it runs from
 * ACLK, so the board needs a 32.768 kHz watch crystal. Kick it from exactly
 * one place: the main loop, after ts_bear_poll(). */
#ifndef TS_F169_WATCHDOG
#define TS_F169_WATCHDOG 0
#endif

/* Configures clocks, the 1 ms timer and the power pins. Call once at start,
 * then enable interrupts. */
void ts_f169_init(void);

ts_platform_t ts_f169_platform(void);

/* Restarts the watchdog timeout. Does nothing if the watchdog is off. */
void ts_f169_watchdog_kick(void);

#endif
