/* SPDX-License-Identifier: MIT */
#include <msp430.h>
#include "ts_f169_console.h"

void ts_f169_console_init(void)
{
    P3SEL |= BIT4 | BIT5;
    P3DIR |= BIT4;
    U0CTL = CHAR | SWRST;
    U0TCTL = SSEL1;
    /* 115200 baud from 8 MHz SMCLK: divider 69.44 = 69 plus modulation. */
    U0BR0 = 69;
    U0BR1 = 0;
    U0MCTL = 0x55;
    ME1 |= UTXE0;
    U0CTL &= (unsigned char)~SWRST;
}

void ts_f169_console_print(const char *text)
{
    while (*text) {
        while (!(IFG1 & UTXIFG0)) { }
        U0TXBUF = (unsigned char)*text++;
    }
}
