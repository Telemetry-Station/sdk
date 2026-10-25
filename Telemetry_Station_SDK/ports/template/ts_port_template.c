/* SPDX-License-Identifier: MIT */
/*
 * Starting point for a new microcontroller. Copy this file, rename the
 * functions, and replace each TODO with your MCU's registers or HAL calls.
 * See docs/PORTING.md for the rules each function must follow.
 *
 * It compiles as-is (CMake builds it) but does nothing useful until filled in.
 */
#include "ts_platform.h"

typedef struct {
    volatile uint32_t tick_ms; /* Incremented by your 1 ms timer interrupt. */
} my_port_t;

static my_port_t my_port;

/* Return the next received byte, or 0 if none. Return -1 if bytes were lost.
 * Fill a ring buffer in your UART receive interrupt, and read from it here. */
static int16_t my_serial_read(void *user, uint8_t *dst, size_t capacity)
{
    (void)user; (void)dst; (void)capacity;
    /* TODO: pop one byte from your receive ring buffer. */
    return 0;
}

/* Queue as many bytes as fit right now and return how many. Never wait. */
static int16_t my_serial_write(void *user, const uint8_t *src, size_t length)
{
    (void)user; (void)src; (void)length;
    /* TODO: push bytes into your transmit ring buffer. */
    return 0;
}

/* Milliseconds since start. Wrap-around is fine. */
static uint32_t my_millis(void *user)
{
    return ((my_port_t *)user)->tick_ms; /* TODO: make this read atomic on 8/16-bit MCUs. */
}

/* Drive the module's ENABLE signal high. */
static bool my_power_start(void *user)
{
    (void)user;
    /* TODO: set your ENABLE pin high. */
    return true;
}

/* Drive ENABLE low. Do not remove the module's supply here. */
static void my_power_stop(void *user)
{
    (void)user;
    /* TODO: set your ENABLE pin low. */
}

/* true: connect the UART pins. false: release them to high impedance so the
 * unpowered module is not back-fed. */
static void my_uart_enable(void *user, bool enable)
{
    (void)user; (void)enable;
    /* TODO: configure the UART and its pins. */
}

/* Read the module's BOOTED signal. */
static bool my_boot_ready(void *user)
{
    (void)user;
    /* TODO: read your BOOTED pin. */
    return false;
}

ts_platform_t my_platform(void)
{
    ts_platform_t port = {0};
    port.user = &my_port;
    port.serial_read = my_serial_read;
    port.serial_write = my_serial_write;
    port.millis = my_millis;
    port.power_start = my_power_start;
    port.power_stop = my_power_stop;
    port.uart_enable = my_uart_enable;
    port.boot_ready = my_boot_ready;
    return port;
}
