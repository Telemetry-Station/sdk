/* SPDX-License-Identifier: MIT */
#ifndef TS_POSIX_H
#define TS_POSIX_H
#include "ts_platform.h"
/* For bench modules ALREADY powered and BOOTED externally.
 * Does not toggle any power rails or GPIO. Never assume host UART levels match.
 */
typedef struct {
    int fd;
    uint32_t start_ms;
} ts_posix_t;
int ts_posix_open(ts_posix_t *dev, const char *tty_path);
void ts_posix_close(ts_posix_t *dev);
ts_platform_t ts_posix_platform(ts_posix_t *dev);
#endif
