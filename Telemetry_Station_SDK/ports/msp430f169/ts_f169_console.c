/* SPDX-License-Identifier: MIT */
#include <msp430.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "ts_f169_console.h"
#include "ts_config.h"
#if TS_ENABLE_CONSOLE

#define CONSOLE_MASK (TS_F169_CONSOLE_RING - 1u)

#if TS_F169_CONSOLE_RING < 2u || (TS_F169_CONSOLE_RING & CONSOLE_MASK) || TS_F169_CONSOLE_RING > 256u
#error TS_F169_CONSOLE_RING must be a power of two no larger than 256
#endif

/* Single producer (main loop), single consumer (USART0 TX interrupt). */
static volatile uint8_t ring[TS_F169_CONSOLE_RING];
static volatile uint8_t head, tail;

void ts_f169_console_init(void)
{
    IE1 &= (unsigned char)~UTXIE0;
    head = tail = 0;
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

bool ts_f169_console_write(const char *text)
{
    size_t length = strlen(text), i;
    uint8_t at = head;
    size_t room = (size_t)((uint8_t)(tail - at - 1u) & CONSOLE_MASK);
    if (length > room) return false;
    for (i = 0; i < length; ++i) {
        ring[at] = (uint8_t)text[i];
        at = (uint8_t)((at + 1u) & CONSOLE_MASK);
    }
    head = at; /* Publish only after the bytes are in place. */
    if (length) IE1 |= UTXIE0;
    return true;
}

void ts_f169_console_print(const char *text)
{
    (void)ts_f169_console_write(text);
}

bool ts_f169_console_idle(void)
{
    return head == tail && (U0TCTL & TXEPT);
}

#if defined(__GNUC__)
__attribute__((interrupt(USART0TX_VECTOR))) void usart0_tx_isr(void)
#else
#pragma vector = USART0TX_VECTOR
__interrupt void usart0_tx_isr(void)
#endif
{
    if (tail != head) {
        U0TXBUF = ring[tail];
        tail = (uint8_t)((tail + 1u) & CONSOLE_MASK);
    } else {
        IE1 &= (unsigned char)~UTXIE0;
    }
}

#else
void ts_f169_console_init(void) { }
bool ts_f169_console_write(const char *text) { (void)text; return true; }
void ts_f169_console_print(const char *text) { (void)text; }
bool ts_f169_console_idle(void) { return true; }
#endif
