/* SPDX-License-Identifier: MIT */
#ifndef TS_GNSS_H
#define TS_GNSS_H
#include "ts_config.h"
#include <stdint.h>
#include <stdbool.h>
/* Optional LEA-M8S UBX NAV-PVT acquisition. No implied UART/power-pin mapping.
 * Host supplies nonblocking one-byte read and reviewed power/isolation control.
 * Configure NAV-PVT output in receiver RAM through the integrator's transport.
 * No flash writes, automatic retries, or GNSS bytes sent to the modem UART. */
typedef struct {
    void *user;
    int (*read_byte)(void *user, uint8_t *byte); /* 1 byte, 0 empty, -1 error */
    bool (*power)(void *user, bool on); /* off must isolate I/O before power */
} ts_gnss_port_t;
typedef enum { TS_GNSS_OFF, TS_GNSS_ACQUIRING, TS_GNSS_FIX,
               TS_GNSS_TIMEOUT, TS_GNSS_IO_ERROR } ts_gnss_status_t;
typedef struct {
    int32_t latitude_e7, longitude_e7;
    uint32_t horizontal_accuracy_mm;
    uint8_t satellites;
} ts_gnss_fix_t;
typedef struct {
    ts_gnss_port_t port;
    ts_gnss_status_t status;
    ts_gnss_fix_t fix;
    uint32_t deadline;
#if TS_ENABLE_GNSS
    uint8_t frame[100]; /* Exact UBX NAV-PVT frame; bounded resynchronization. */
    uint8_t used;
#endif
} ts_gnss_t;
bool ts_gnss_init(ts_gnss_t *g, const ts_gnss_port_t *port);
bool ts_gnss_begin(ts_gnss_t *g, uint32_t now, uint32_t budget_ms);
void ts_gnss_poll(ts_gnss_t *g, uint32_t now);
void ts_gnss_cancel(ts_gnss_t *g);
#endif
