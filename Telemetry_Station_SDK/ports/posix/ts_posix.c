/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include "ts_posix.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static uint32_t ticks(void *user)
{
    struct timespec t;
    (void)user;
    (void)clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint32_t)((uint64_t)t.tv_sec * 1000u + (uint32_t)t.tv_nsec / 1000000u);
}
static int16_t rd(void *user, uint8_t *dst, size_t cap)
{
    ts_posix_t *p = (ts_posix_t*)user;
    ssize_t n = read(p->fd, dst, cap);
    if (n >= 0) return (int16_t)(n > INT16_MAX ? INT16_MAX : n);
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return 0;
    return -1;
}
static int16_t wr(void *user, const uint8_t *src, size_t len)
{
    ts_posix_t *p = (ts_posix_t*)user;
    ssize_t n = write(p->fd, src, len);
    if (n >= 0) return (int16_t)(n > INT16_MAX ? INT16_MAX : n);
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return 0;
    return -1;
}
static bool externally_powered(void *user) { (void)user; return true; }
static void noop_stop(void *user) { (void)user; }
static void noop_uart(void *user, bool enabled) { (void)user; (void)enabled; }
static bool externally_booted(void *user) { (void)user; return true; }

ts_platform_t ts_posix_platform(ts_posix_t *dev)
{
    ts_platform_t p = {0};
    p.user=dev; p.serial_read=rd; p.serial_write=wr; p.millis=ticks;
    p.power_start=externally_powered; p.power_stop=noop_stop;
    p.uart_enable=noop_uart; p.boot_ready=externally_booted;
    return p;
}
int ts_posix_open(ts_posix_t *dev, const char *tty_path)
{
    struct termios opt;
    if (!dev || !tty_path) return -1;
#ifndef B230400
    /* Some macOS SDKs lack a 230400 baud termios constant; explicitly configure
     * IOSSIOSPEED or use a USB-UART adapter with documented supported rates. */
    (void)opt;
    return -1;
#else
    dev->fd = open(tty_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (dev->fd < 0) return -1;
    if (tcgetattr(dev->fd, &opt) || cfsetispeed(&opt,B230400) ||
        cfsetospeed(&opt,B230400)) { ts_posix_close(dev); return -1; }
    opt.c_iflag &= (tcflag_t)~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    opt.c_oflag &= (tcflag_t)~OPOST;
    opt.c_lflag &= (tcflag_t)~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    opt.c_cflag |= CLOCAL | CREAD | CS8;
    opt.c_cflag &= (tcflag_t)~(PARENB | CSTOPB);
    if (tcsetattr(dev->fd,TCSANOW,&opt)) { ts_posix_close(dev); return -1; }
    return 0;
#endif
}
void ts_posix_close(ts_posix_t *dev)
{
    if (dev && dev->fd >= 0) { close(dev->fd); dev->fd=-1; }
}
