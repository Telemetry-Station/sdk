/* SPDX-License-Identifier: MIT */
/*
 * Host-side stand-in for the device header of the TI MSP430F169 toolchain.
 *
 * It exists only so the MSP430 port and example can be compiled on a normal
 * computer, to catch syntax, type and warning problems on every build. The
 * register values here are placeholders. Never use this header for a real
 * target: build the port with your MSP430 toolchain, which provides the real
 * <msp430.h>.
 */
#ifndef TS_STUB_MSP430_H
#define TS_STUB_MSP430_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Included now so no system header sees the __attribute__ override below. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TS_STUB_REG8(name) extern volatile unsigned char name
#define TS_STUB_REG16(name) extern volatile unsigned int name

TS_STUB_REG8(P1IN);  TS_STUB_REG8(P1OUT); TS_STUB_REG8(P1DIR);
TS_STUB_REG8(P3SEL); TS_STUB_REG8(P3DIR);
TS_STUB_REG8(IE2);   TS_STUB_REG8(IFG1);
TS_STUB_REG8(ME1);   TS_STUB_REG8(ME2);
TS_STUB_REG8(BCSCTL1); TS_STUB_REG8(BCSCTL2);
TS_STUB_REG8(U0CTL); TS_STUB_REG8(U0TCTL); TS_STUB_REG8(U0BR0);
TS_STUB_REG8(U0BR1); TS_STUB_REG8(U0MCTL); TS_STUB_REG8(U0TXBUF);
TS_STUB_REG8(U1CTL); TS_STUB_REG8(U1TCTL); TS_STUB_REG8(U1BR0);
TS_STUB_REG8(U1BR1); TS_STUB_REG8(U1MCTL); TS_STUB_REG8(U1TXBUF);
TS_STUB_REG8(U1RXBUF);
TS_STUB_REG16(WDTCTL); TS_STUB_REG16(TACTL);
TS_STUB_REG16(TACCTL0); TS_STUB_REG16(TACCR0);

#define BIT0 0x01u
#define BIT1 0x02u
#define BIT4 0x10u
#define BIT5 0x20u
#define BIT6 0x40u
#define BIT7 0x80u

#define CHAR 0x10u
#define SWRST 0x01u
#define SSEL1 0x20u
#define OFIFG 0x02u
#define UTXIFG0 0x80u
#define UTXE0 0x80u
#define URXE1 0x10u
#define UTXE1 0x20u
#define URXIE1 0x10u
#define UTXIE1 0x20u
#define XT2OFF 0x80u
#define SELM_2 0x80u
#define SELS 0x08u
#define TASSEL_2 0x0200u
#define ID_3 0x00C0u
#define MC_1 0x0010u
#define TACLR 0x0004u
#define CCIE 0x0010u
#define WDTPW 0x5A00u
#define WDTHOLD 0x0080u
#define WDT_ARST_1000 0x5A1Cu
#define GIE 0x0008u
#define LPM0_bits 0x0010u

#define TIMERA0_VECTOR 12
#define USART1RX_VECTOR 6
#define USART1TX_VECTOR 4

unsigned int __get_SR_register(void);
void __enable_interrupt(void);
void __disable_interrupt(void);
void __bis_SR_register(unsigned int bits);
void __bic_SR_register_on_exit(unsigned int bits);

/* The interrupt attribute only means something on an MSP430 compiler. */
#define __interrupt
#undef __attribute__
#define __attribute__(x)

#endif
