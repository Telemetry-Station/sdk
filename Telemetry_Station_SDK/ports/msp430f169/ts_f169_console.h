/* SPDX-License-Identifier: MIT */
#ifndef TS_F169_CONSOLE_H
#define TS_F169_CONSOLE_H

#include <stdbool.h>

/*
 * Text console on USART0 (P3.4 = TXD0, P3.5 = RXD0) at 115200 baud 8N1, for
 * a USB-serial adapter. Transmission is interrupt driven from a ring buffer,
 * so printing never blocks the modem driver.
 *
 * Build with -DTS_ENABLE_CONSOLE=1 to enable it; otherwise every call is a
 * no-op that reports success.
 */

/* Ring size in bytes. Power of two, 256 or less. */
#ifndef TS_F169_CONSOLE_RING
#define TS_F169_CONSOLE_RING 128u
#endif

void ts_f169_console_init(void);

/* Queues all of text, or nothing if it does not fit right now (retry later).
 * Text longer than the ring can never fit and returns false. */
bool ts_f169_console_write(const char *text);

/* Best effort: like ts_f169_console_write() but drops text that does not fit. */
void ts_f169_console_print(const char *text);

/* True once every queued byte has left the transmit shift register. */
bool ts_f169_console_idle(void);

#endif
