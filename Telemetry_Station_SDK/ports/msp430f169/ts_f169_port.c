/* SPDX-License-Identifier: MIT */
#include <msp430.h>
#include "ts_f169_port.h"

/*
 * The interrupt handlers only move bytes and count milliseconds. All parsing
 * and modem logic runs in ts_bear_poll() in the main loop.
 */

#define RING_MASK (TS_F169_RING_SIZE - 1u)

#if (TS_F169_RING_SIZE & RING_MASK) || TS_F169_RING_SIZE > 256u
#error TS_F169_RING_SIZE must be a power of two no larger than 256
#endif

static volatile uint32_t tick_ms;

/* Single producer, single consumer rings. */
static volatile uint8_t rx_ring[TS_F169_RING_SIZE];
static volatile uint8_t rx_head, rx_tail;
static volatile uint8_t rx_overflow;
static volatile uint8_t tx_ring[TS_F169_RING_SIZE];
static volatile uint8_t tx_head, tx_tail;

void ts_f169_init(void)
{
    volatile unsigned int i;

#if TS_F169_WATCHDOG
    WDTCTL = WDT_ARST_1000; /* Reset if not kicked for ~1 s (ACLK). */
#else
    WDTCTL = WDTPW | WDTHOLD;
#endif

    /* MCLK and SMCLK from the 8 MHz XT2 crystal. */
    BCSCTL1 &= (uint8_t)~XT2OFF;
    do {
        IFG1 &= (uint8_t)~OFIFG;
        for (i = 0xFF; i; --i) { }
    } while (IFG1 & OFIFG);
    BCSCTL2 = SELM_2 | SELS;

    /* Timer_A: 8 MHz / 8 = 1 MHz, so 1000 counts = 1 ms. */
    TACTL = TASSEL_2 | ID_3 | TACLR;
    TACCR0 = 999;
    TACCTL0 = CCIE;
    TACTL |= MC_1;

    TS_F169_ENABLE_OUT &= (unsigned char)~TS_F169_ENABLE_BIT;
    TS_F169_ENABLE_DIR |= TS_F169_ENABLE_BIT;
    TS_F169_BOOTED_DIR &= (unsigned char)~TS_F169_BOOTED_BIT;
}

void ts_f169_watchdog_kick(void)
{
#if TS_F169_WATCHDOG
    WDTCTL = WDT_ARST_1000;
#endif
}

static uint32_t port_millis(void *user)
{
    uint32_t copy;
    unsigned int interrupts_were_on = __get_SR_register() & GIE;
    (void)user;
    __disable_interrupt(); /* A 32-bit read takes several instructions. */
    copy = tick_ms;
    if (interrupts_were_on) __enable_interrupt();
    return copy;
}

static void port_uart_enable(void *user, bool enable)
{
    (void)user;
    IE2 &= (unsigned char)~(URXIE1 | UTXIE1);
    U1CTL |= SWRST;
    if (!enable) {
        /* Both UART pins become inputs so the modem is never back-powered. */
        P3SEL &= (unsigned char)~(BIT6 | BIT7);
        P3DIR &= (unsigned char)~(BIT6 | BIT7);
        return;
    }
    P3SEL |= BIT6 | BIT7;
    P3DIR |= BIT6;
    P3DIR &= (unsigned char)~BIT7;

    /* 8N1 at 230400 baud from 8 MHz: divider 34.72 = 34 plus modulation. */
    U1CTL = CHAR | SWRST;
    U1TCTL = SSEL1;
    U1BR0 = 34;
    U1BR1 = 0;
    U1MCTL = 0x77;
    ME2 |= UTXE1 | URXE1;
    U1CTL &= (unsigned char)~SWRST;

    rx_head = rx_tail = rx_overflow = 0;
    tx_head = tx_tail = 0;
    IE2 |= URXIE1;
}

static int16_t port_serial_read(void *user, uint8_t *dst, size_t capacity)
{
    (void)user;
    if (rx_overflow) { /* Bytes were lost; the driver treats this as an error. */
        rx_overflow = 0;
        return -1;
    }
    if (!capacity || rx_head == rx_tail) return 0;
    *dst = rx_ring[rx_tail];
    rx_tail = (uint8_t)((rx_tail + 1u) & RING_MASK);
    return 1;
}

static int16_t port_serial_write(void *user, const uint8_t *src, size_t length)
{
    size_t count = 0;
    (void)user;
    while (count < length) {
        uint8_t next = (uint8_t)((tx_head + 1u) & RING_MASK);
        if (next == tx_tail) break; /* Full: the driver retries the rest. */
        tx_ring[tx_head] = src[count++];
        tx_head = next;
    }
    if (tx_tail != tx_head) IE2 |= UTXIE1;
    return (int16_t)count;
}

static bool port_power_start(void *user)
{
    (void)user;
    TS_F169_ENABLE_OUT |= TS_F169_ENABLE_BIT;
    return true;
}

static void port_power_stop(void *user)
{
    (void)user;
    TS_F169_ENABLE_OUT &= (unsigned char)~TS_F169_ENABLE_BIT;
}

static bool port_boot_ready(void *user)
{
    (void)user;
    return (TS_F169_BOOTED_IN & TS_F169_BOOTED_BIT) != 0;
}

ts_platform_t ts_f169_platform(void)
{
    ts_platform_t port = {0};
    port.serial_read = port_serial_read;
    port.serial_write = port_serial_write;
    port.millis = port_millis;
    port.power_start = port_power_start;
    port.power_stop = port_power_stop;
    port.uart_enable = port_uart_enable;
    port.boot_ready = port_boot_ready;
    return port;
}

/* ---- Interrupt handlers ---- */

#if defined(__GNUC__)
__attribute__((interrupt(TIMERA0_VECTOR))) void timer_a0_isr(void)
#else
#pragma vector = TIMERA0_VECTOR
__interrupt void timer_a0_isr(void)
#endif
{
    ++tick_ms;
    __bic_SR_register_on_exit(LPM0_bits); /* Wake the main loop. */
}

#if defined(__GNUC__)
__attribute__((interrupt(USART1RX_VECTOR))) void usart1_rx_isr(void)
#else
#pragma vector = USART1RX_VECTOR
__interrupt void usart1_rx_isr(void)
#endif
{
    uint8_t byte = U1RXBUF;
    uint8_t next = (uint8_t)((rx_head + 1u) & RING_MASK);
    if (next == rx_tail) {
        rx_overflow = 1;
    } else {
        rx_ring[rx_head] = byte;
        rx_head = next;
    }
    __bic_SR_register_on_exit(LPM0_bits);
}

#if defined(__GNUC__)
__attribute__((interrupt(USART1TX_VECTOR))) void usart1_tx_isr(void)
#else
#pragma vector = USART1TX_VECTOR
__interrupt void usart1_tx_isr(void)
#endif
{
    if (tx_tail != tx_head) {
        U1TXBUF = tx_ring[tx_tail];
        tx_tail = (uint8_t)((tx_tail + 1u) & RING_MASK);
    } else {
        IE2 &= (unsigned char)~UTXIE1;
    }
}
