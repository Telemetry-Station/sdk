/* SPDX-License-Identifier: MIT */
/*
 * Runs the real MSP430F169 port, console and example firmware (main.c) on the
 * host. The registers in tests/stubs/msp430.h are backed by variables here,
 * and a small board model plays the hardware around them:
 *
 *   Timer_A    fires timer_a0_isr() once per simulated millisecond
 *   USART1     moves bytes between the port's rings and the simulated BEAR
 *              modem at 230400 baud (about 23 bytes per millisecond)
 *   USART0     drains the console ring at 115200 baud (11 bytes per ms) into
 *              a capture buffer: exactly what a serial terminal would show
 *   ENABLE     P1.0 output; the module raises BOOTED (P1.1) after it
 *
 * Each pass of the firmware's main loop ends in __bis_SR_register(LPM0), which
 * is where the board model advances time. The scenarios are what a first-time
 * user sees after flashing: a working module, no module, swapped UART wires,
 * and a BOOTED wire stuck high.
 *
 * Build twice: TS_TOPIC_ID=0 (activation flow) and a real topic (send flow).
 * Run "test_f169_firmware_0 console" to print the raw console capture.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim_modem.h"
#include "ts_activation.h"

#ifndef REPORT_INTERVAL_MS
#define REPORT_INTERVAL_MS 60000UL /* Shortened so soak cycles run quickly. */
#endif

/* The real firmware, with its main() renamed so this file can drive it. The
 * stop hook lets main() return normally (coverage counts stay exact). */
static bool firmware_stop(void);
#define TS_FIRMWARE_STOP() firmware_stop()
#define main firmware_main
#include "../examples/msp430f169/main.c"
#undef main

/* ---- Register file behind the stub ---- */

volatile unsigned char P1IN, P1OUT, P1DIR, P3SEL, P3DIR, IE1, IE2, ME1, ME2;
static volatile unsigned char ifg1;
volatile unsigned char BCSCTL1, BCSCTL2;
volatile unsigned char U0CTL, U0TCTL, U0BR0, U0BR1, U0MCTL, U0TXBUF;
volatile unsigned char U1CTL, U1TCTL, U1BR0, U1BR1, U1MCTL, U1TXBUF, U1RXBUF;
volatile unsigned int WDTCTL, TACTL, TACCTL0, TACCR0;

/* Interrupt handlers in ts_f169_port.c and ts_f169_console.c. */
void timer_a0_isr(void);
void usart1_rx_isr(void);
void usart1_tx_isr(void);
#if TS_ENABLE_CONSOLE
void usart0_tx_isr(void);
#endif

static unsigned int status_register;
static unsigned int lpm_exits;

unsigned int __get_SR_register(void) { return status_register; }
void __enable_interrupt(void) { status_register |= GIE; }
void __disable_interrupt(void) { status_register &= ~GIE; }
void __bic_SR_register_on_exit(unsigned int bits) { (void)bits; ++lpm_exits; }

/* ---- Board model ---- */

typedef struct {
    sim_modem_t m;
    ts_platform_t modem;
    bool connected;        /* BEAR module plugged in. */
    bool swapped;          /* UART TX/RX wires crossed the wrong way. */
    bool booted_stuck;     /* BOOTED wire shorted high. */
    bool xt2_dead;         /* 8 MHz crystal missing: OSCFault never clears. */
    unsigned xt2_spins;
    uint32_t boot_delay, fall_delay;
    uint32_t now, enable_edge, enable_high_ms;
    bool enable;
    char console[64 * 1024];
    size_t console_len;
    const char *stop_text;
    unsigned stop_count;
    uint32_t limit_ms, stop_ms;
    unsigned enable_rises;
    bool stopped;
} board_t;

static board_t board;

volatile unsigned char *ts_stub_ifg1(void)
{
    if (board.xt2_dead) ifg1 |= OFIFG;
    return &ifg1;
}

static bool firmware_stop(void)
{
    /* With no clock the firmware spins without sleeping; give up after a while. */
    if (board.xt2_dead && ++board.xt2_spins > 1000u) return true;
    return board.stopped;
}

static void reset_registers(void)
{
    P1IN = P1OUT = P1DIR = P3SEL = P3DIR = IE1 = IE2 = ifg1 = ME1 = ME2 = 0;
    BCSCTL1 = BCSCTL2 = 0;
    U0CTL = U0TCTL = U0BR0 = U0BR1 = U0MCTL = U0TXBUF = 0;
    U1CTL = U1TCTL = U1BR0 = U1BR1 = U1MCTL = U1TXBUF = U1RXBUF = 0;
    WDTCTL = TACTL = TACCTL0 = TACCR0 = 0;
    status_register = 0;
}

static void board_init(void)
{
    memset(&board, 0, sizeof board);
    sim_init(&board.m);
    board.modem = sim_platform(&board.m);
    board.connected = true;
    board.boot_delay = 1500;
    board.fall_delay = 700;
    board.limit_ms = 10u * 60u * 1000u;
    reset_registers();
}

static unsigned count_of(const char *haystack, const char *needle)
{
    unsigned n = 0;
    while ((haystack = strstr(haystack, needle)) != NULL) { ++n; haystack += strlen(needle); }
    return n;
}

static void model_booted(void)
{
    bool enable = (P1DIR & BIT0) && (P1OUT & BIT0);
    if (enable != board.enable) {
        board.enable = enable;
        board.enable_edge = board.now;
        if (enable) board.enable_rises++;
    }
    if (board.enable) board.enable_high_ms++;
    if (board.booted_stuck) {
        P1IN |= BIT1;
    } else if (!board.connected) {
        P1IN &= (unsigned char)~BIT1;
    } else if (enable && board.now - board.enable_edge >= board.boot_delay) {
        P1IN |= BIT1;
    } else if (!enable && board.now - board.enable_edge >= board.fall_delay) {
        P1IN &= (unsigned char)~BIT1;
    }
}

static bool modem_uart_live(void)
{
    /* Both pins on the peripheral, USART1 out of reset and enabled. */
    return (P3SEL & (BIT6 | BIT7)) == (BIT6 | BIT7) && (P3DIR & BIT6) &&
           !(P3DIR & BIT7) && !(U1CTL & SWRST) && (ME2 & (UTXE1 | URXE1)) == (UTXE1 | URXE1);
}

static void model_modem_uart(void)
{
    unsigned budget;
    /* MCU -> modem. The ISR either loads U1TXBUF or switches itself off. */
    for (budget = 23; budget && (IE2 & UTXIE1); --budget) {
        uint8_t byte;
        usart1_tx_isr();
        if (!(IE2 & UTXIE1)) break;
        byte = U1TXBUF;
        assert(modem_uart_live()); /* Never drive TXD while isolated. */
        if (board.connected && !board.swapped)
            assert(board.modem.serial_write(board.modem.user, &byte, 1) == 1);
    }
    /* Modem -> MCU. */
    if (!board.connected || board.swapped || !(P1IN & BIT1)) return;
    for (budget = 23; budget && modem_uart_live() && (IE2 & URXIE1); --budget) {
        uint8_t byte;
        if (board.modem.serial_read(board.modem.user, &byte, 1) != 1) break;
        U1RXBUF = byte;
        usart1_rx_isr();
    }
}

static void model_console(void)
{
    unsigned budget;
    if (!(IE1 & UTXIE0)) {
        U0TCTL |= TXEPT; /* Shift register empty. */
        return;
    }
#if !TS_ENABLE_CONSOLE
    assert(!"console interrupt enabled in a build without a console");
#else
    U0TCTL &= (unsigned char)~TXEPT;
    /* The console must be configured before anything is queued. */
    assert((P3SEL & BIT4) && (P3DIR & BIT4) && (ME1 & UTXE0) && !(U0CTL & SWRST));
    assert(U0BR0 == 69 && U0BR1 == 0); /* 115200 baud from 8 MHz. */
    for (budget = 11; budget && (IE1 & UTXIE0); --budget) {
        usart0_tx_isr();
        if (!(IE1 & UTXIE0)) break;
        assert(board.console_len + 1u < sizeof board.console);
        board.console[board.console_len++] = (char)U0TXBUF;
        board.console[board.console_len] = '\0';
    }
#endif
}

/* One millisecond of hardware time; called each time the firmware sleeps. */
void __bis_SR_register(unsigned int bits)
{
    status_register |= bits & GIE;
    assert(status_register & GIE); /* Sleeping without interrupts would hang. */
    board.now++;
    model_booted();
    if ((TACTL & MC_1) && (TACCTL0 & CCIE)) timer_a0_isr();
    model_modem_uart();
    model_console();
    assert(board.now < board.limit_ms);
    /* Stop once the text has appeared and the console has gone quiet. */
    if (board.stop_text && !(IE1 & UTXIE0) &&
        count_of(board.console, board.stop_text) >= board.stop_count)
        board.stopped = true;
    if (board.stop_ms && board.now >= board.stop_ms) board.stopped = true;
}

/* Boots the firmware and runs it until stop_text has appeared count times. */
static int run_firmware(const char *stop_text, unsigned count)
{
    int code;
    board.stop_text = stop_text;
    board.stop_count = count;
    model_booted(); /* Pin levels as the firmware first sees them. */
    code = firmware_main();
    assert(board.stopped || board.xt2_dead);
    return code;
}

/* The console without ANSI colour codes, for matching. */
static const char *plain(void)
{
    static char text[sizeof board.console];
    size_t i, n = 0;
    for (i = 0; i < board.console_len; ++i) {
        if (board.console[i] == '\x1b') {
            while (i < board.console_len && board.console[i] != 'm') ++i;
            continue;
        }
        text[n++] = board.console[i];
    }
    text[n] = '\0';
    return text;
}

static void check_lines(void)
{
    /* Every line is complete: no torn or dropped output. */
    const char *p = board.console;
    while ((p = strchr(p, '\n')) != NULL) {
        assert(p > board.console && p[-1] == '\r');
        ++p;
    }
    assert(board.console_len >= 2 && board.console[board.console_len - 1] == '\n');
}

#define SEE(text) do { if (!strstr(plain(), text)) { \
    fputs(plain(), stderr); fprintf(stderr, "missing: %s\n", text); assert(0); } } while (0)
#define NOT_SEE(text) do { if (strstr(plain(), text)) { \
    fputs(plain(), stderr); fprintf(stderr, "unexpected: %s\n", text); assert(0); } } while (0)

/* ---- Firmware scenarios ---- */

static void test_firmware_first_flash(void)
{
    board_init();
    assert(run_firmware("Cycle 3 ", 1) == 0);
    check_lines();
    SEE("Telemetry Station SDK v" TS_SDK_VERSION_STRING "  |  BEAR Module 9704 checkout");
    SEE(" Board    MSP430F169, XT2 8 MHz");
    SEE(" Wiring   ENABLE P1.0, BOOTED P1.1, UART1 TX P3.6 / RX P3.7");
    SEE("PASS   Module off at reset  BOOTED low");
    SEE("PASS   Module boot          BOOTED high after 150");
    SEE("PASS   Serial link          modem replied in");
    SEE("PASS   SIM                  internal SIM selected");
    SEE("PASS   Radio active         operational state active");
    SEE("PASS   IMEI                 " SIM_IMEI);
    SEE("PASS   Satellite signal     4/5 bars");
    SEE("PASS   Power down           BOOTED low after");
    SEE(" RESULT   PASS");
    SEE("Cycle 2  PASS");
    SEE("Cycle 3  PASS");
    SEE("(3/3 cycles passed)");
    NOT_SEE("CHECK");
#if TS_TOPIC_ID >= 64u
    {
        char topic[32];
        snprintf(topic, sizeof topic, " Topic    %u\r\n", (unsigned)TS_TOPIC_ID);
        SEE(topic);
    }
    SEE("PASS   Message queued       accepted as message 9");
    SEE("PASS   Delivery             confirmed by Iridium");
    assert(count_of(board.m.sent_log, "PUT messageOriginate ") == 3u);
    NOT_SEE("Next steps");
#else
    SEE(" Topic    not set; send test skipped");
    SEE("SKIP   Message queued       TS_TOPIC_ID not set");
    SEE("Activate IMEI " SIM_IMEI " at " TS_ACTIVATE_URL);
    assert(!strstr(board.m.sent_log, "messageOriginate"));
#endif
    /* Default build prints colour; the module is off between cycles. */
    assert(strstr(board.console, "\x1b[1;32mPASS\x1b[0m"));
    assert(board.enable_high_ms < board.now / 10u);
    assert(!(P1OUT & BIT0) && !(P3SEL & (BIT6 | BIT7)));
    assert(lpm_exits > 0u);
}

static void test_firmware_no_module(void)
{
    board_init();
    board.connected = false;
    run_firmware(" RESULT ", 1);
    check_lines();
    SEE("WAIT   Module boot          waiting for BOOTED to rise (15 s)");
    SEE("CHECK  Module boot          BOOTED never rose; check supply and ENABLE");
    SEE("CHECK  Power down           BOOTED low, ENABLE held; power-cycle board");
    SEE(" RESULT   CHECK");
    NOT_SEE("Next steps");
    /* The module was never driven through the UART while it was absent. */
    assert(!(P3SEL & (BIT6 | BIT7)));
}

static void test_firmware_swapped_uart(void)
{
    board_init();
    board.swapped = true;
    run_firmware(" RESULT ", 1);
    check_lines();
    SEE("PASS   Module boot");
    SEE("CHECK  Serial link          no reply; check TX/RX crossover and baud");
    SEE("PASS   Power down           module off after fault");
    SEE(" RESULT   CHECK");
    assert(!(P1OUT & BIT0) && !(P1IN & BIT1));
}

static void test_firmware_booted_stuck_high(void)
{
    board_init();
    board.booted_stuck = true;
    run_firmware(" RESULT ", 1);
    check_lines();
    SEE("WAIT   Module off at reset  waiting for BOOTED to fall (15 s)");
    SEE("CHECK  Module off at reset  BOOTED stuck high; check BOOTED wiring");
    SEE(" RESULT   CHECK");
    assert(!(P1OUT & BIT0)); /* ENABLE never raised onto a live module. */
}

/* Built with TS_ENABLE_CONSOLE=0: the same checkout cycles, silently. */
static void test_firmware_headless(void)
{
    board_init();
    board.stop_ms = 2u * REPORT_INTERVAL_MS + 5000u;
    assert(run_firmware(NULL, 0) == 0);
    assert(board.console_len == 0u);
    assert(!(ME1 & UTXE0) && !(P3SEL & (BIT4 | BIT5)) && !(IE1 & UTXIE0));
    assert(board.enable_rises == 3u);
    assert(strstr(board.m.sent_log, "PUT messageOriginate "));
    assert(!(P1OUT & BIT0) && !(P1IN & BIT1));
}

static void test_firmware_dead_crystal(void)
{
    board_init();
    board.xt2_dead = true;
    assert(run_firmware("never", 1) == 1);
    /* Without a verified clock nothing is started: no UARTs, no module power. */
    assert(board.console_len == 0u && board.now == 0u);
    assert(!(P1OUT & BIT0) && !(P3SEL & (BIT4 | BIT5 | BIT6 | BIT7)));
    assert(!(ME1 & UTXE0) && !(TACTL & MC_1) && !(status_register & GIE));
    assert(BCSCTL2 == 0u); /* MCLK stays on the DCO. */
}

/* ---- Port and console units ---- */

#if TS_ENABLE_CONSOLE
static void drain_console(char *out, size_t cap)
{
    size_t n = 0;
    while (IE1 & UTXIE0) {
        usart0_tx_isr();
        if (!(IE1 & UTXIE0)) break;
        assert(n + 1u < cap);
        out[n++] = (char)U0TXBUF;
    }
    out[n] = '\0';
}

static void test_console_ring(void)
{
    static char text[TS_F169_CONSOLE_RING + 1u], out[512];
    reset_registers();
    ts_f169_console_init();
    assert(P3SEL & BIT4 && P3DIR & BIT4 && ME1 & UTXE0 && !(U0CTL & SWRST));
    assert(!(IE1 & UTXIE0));

    /* Empty writes succeed and do not start the transmitter. */
    assert(ts_f169_console_write(""));
    assert(!(IE1 & UTXIE0));
    U0TCTL |= TXEPT;
    assert(ts_f169_console_idle());

    /* All or nothing: the ring holds RING - 1 bytes. */
    memset(text, 'a', TS_F169_CONSOLE_RING - 1u);
    text[TS_F169_CONSOLE_RING - 1u] = '\0';
    assert(ts_f169_console_write(text));
    assert(IE1 & UTXIE0);
    assert(!ts_f169_console_idle());
    assert(!ts_f169_console_write("b"));
    drain_console(out, sizeof out);
    assert(strlen(out) == TS_F169_CONSOLE_RING - 1u && !strcmp(out, text));
    assert(ts_f169_console_idle());
    U0TCTL &= (unsigned char)~TXEPT;
    assert(!ts_f169_console_idle()); /* Last byte still shifting out. */
    U0TCTL |= TXEPT;

    /* Too long ever to fit: refused, nothing queued. */
    memset(text, 'c', TS_F169_CONSOLE_RING);
    text[TS_F169_CONSOLE_RING] = '\0';
    assert(!ts_f169_console_write(text));
    assert(!(IE1 & UTXIE0));

    /* Wrap-around keeps byte order; partial drains leave room for more. */
    {
        unsigned round;
        char expect[512] = "";
        for (round = 0; round < 20u; ++round) {
            char line[40];
            snprintf(line, sizeof line, "line %u of twenty\r\n", round);
            while (!ts_f169_console_write(line)) {
                size_t at = strlen(out);
                usart0_tx_isr(); /* One byte leaves. */
                if (IE1 & UTXIE0) { out[at] = (char)U0TXBUF; out[at + 1] = '\0'; }
            }
            strcat(expect, line);
            if (round == 0u) out[0] = '\0';
        }
        {
            size_t at = strlen(out);
            drain_console(out + at, sizeof out - at);
        }
        assert(!strcmp(out, expect));
    }

    /* Best-effort print drops what does not fit. */
    memset(text, 'd', TS_F169_CONSOLE_RING - 1u);
    text[TS_F169_CONSOLE_RING - 1u] = '\0';
    ts_f169_console_print(text);
    ts_f169_console_print("lost");
    drain_console(out, sizeof out);
    assert(!strcmp(out, text));
}

#endif

static void test_modem_port(void)
{
    ts_platform_t port;
    uint8_t byte, big[300];
    unsigned i;
    int16_t n;

    reset_registers();
    P1OUT = BIT0; /* Garbage left by a bootloader. */
    P3SEL = BIT6 | BIT7;
    assert(ts_f169_init());
    assert(WDTCTL == (WDTPW | WDTHOLD));
    assert((P1DIR & BIT0) && !(P1OUT & BIT0)); /* ENABLE: output, low. */
    assert(!(P1DIR & BIT1));                   /* BOOTED: input. */
    assert(!(P3SEL & (BIT6 | BIT7)));          /* Modem UART isolated. */
    assert(TACCR0 == 999 && (TACTL & MC_1) && (TACCTL0 & CCIE));
    port = ts_f169_platform();
    assert(port.serial_read && port.serial_write && port.millis && port.power_start &&
           port.power_stop && port.uart_enable && port.boot_ready);

    /* 1 ms tick; interrupts restored as they were. */
    {
        uint32_t t0 = port.millis(port.user);
        timer_a0_isr(); timer_a0_isr();
        assert(port.millis(port.user) - t0 == 2u);
        status_register = GIE;
        (void)port.millis(port.user);
        assert(status_register & GIE);
        status_register = 0;
        (void)port.millis(port.user);
        assert(!(status_register & GIE));
    }

    /* Power pins. */
    assert(port.power_start(port.user) && (P1OUT & BIT0));
    assert(!port.boot_ready(port.user));
    P1IN = BIT1;
    assert(port.boot_ready(port.user));
    port.power_stop(port.user);
    assert(!(P1OUT & BIT0));

    /* UART on: pins and 230400 baud; off: both pins inputs. */
    port.uart_enable(port.user, true);
    assert((P3SEL & (BIT6 | BIT7)) == (BIT6 | BIT7) && (P3DIR & BIT6) && !(P3DIR & BIT7));
    assert(U1BR0 == 34 && U1BR1 == 0 && !(U1CTL & SWRST) && (IE2 & URXIE1));

    /* Writes fill the ring and report partial progress when full. */
    memset(big, 'x', sizeof big);
    n = port.serial_write(port.user, big, sizeof big);
    assert(n == (int16_t)(TS_F169_RING_SIZE - 1u) && (IE2 & UTXIE1));
    assert(port.serial_write(port.user, big, 1) == 0);
    for (i = 0; i < 5u; ++i) usart1_tx_isr();
    assert(port.serial_write(port.user, big, sizeof big) == 5);
    while (IE2 & UTXIE1) usart1_tx_isr();
    assert(port.serial_write(port.user, big, 0) == 0 && !(IE2 & UTXIE1));

    /* Reads: in order, empty, and overflow reported once as an error. */
    assert(port.serial_read(port.user, &byte, 1) == 0);
    U1RXBUF = 'k'; usart1_rx_isr();
    assert(port.serial_read(port.user, &byte, 0) == 0);
    assert(port.serial_read(port.user, &byte, 1) == 1 && byte == 'k');
    for (i = 0; i < TS_F169_RING_SIZE + 3u; ++i) { U1RXBUF = (unsigned char)i; usart1_rx_isr(); }
    assert(port.serial_read(port.user, &byte, 1) == -1);
    assert(port.serial_read(port.user, &byte, 1) == 1 && byte == 0);

    /* Isolation resets the rings and releases the pins. */
    port.uart_enable(port.user, false);
    assert(!(P3SEL & (BIT6 | BIT7)) && !(P3DIR & (BIT6 | BIT7)));
    assert(!(IE2 & (URXIE1 | UTXIE1)) && (U1CTL & SWRST));
    port.uart_enable(port.user, true);
    assert(port.serial_read(port.user, &byte, 1) == 0);
    ts_f169_watchdog_kick();
}

#if !TS_ENABLE_CONSOLE
static void test_console_stubs(void)
{
    reset_registers();
    ts_f169_console_init();
    assert(ts_f169_console_write("ignored\r\n"));
    ts_f169_console_print("ignored\r\n");
    assert(ts_f169_console_idle());
    assert(!ME1 && !P3SEL && !IE1 && !U0CTL);
}
#endif

int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "console")) {
        board_init();
        if (argc > 2 && !strcmp(argv[2], "nomodule")) board.connected = false;
        if (argc > 2 && !strcmp(argv[2], "swapped")) board.swapped = true;
        run_firmware(argc > 2 ? " RESULT " : "Cycle 3 ", 1);
        fwrite(board.console, 1, board.console_len, stdout);
        return 0;
    }
    test_modem_port();
#if TS_ENABLE_CONSOLE
    test_console_ring();
    test_firmware_first_flash();
    test_firmware_no_module();
    test_firmware_swapped_uart();
    test_firmware_booted_stuck_high();
#else
    test_console_stubs();
    test_firmware_headless();
#endif
    test_firmware_dead_crystal();
    printf("MSP430F169 port and firmware tests passed (TS_TOPIC_ID=%u)\n", (unsigned)TS_TOPIC_ID);
    return 0;
}
