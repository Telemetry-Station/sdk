/* SPDX-License-Identifier: MIT */
#ifndef TS_F169_CONSOLE_H
#define TS_F169_CONSOLE_H

/*
 * A minimal text console on USART0 (P3.4 = TXD0, P3.5 = RXD0) at 115200 baud,
 * for showing the IMEI and status messages through a USB-serial adapter.
 * Output blocks until sent, so keep it out of time-critical code.
 */
void ts_f169_console_init(void);
void ts_f169_console_print(const char *text);

#endif
