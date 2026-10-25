/* SPDX-License-Identifier: MIT */
/* Optional macOS/Linux bench program. Hardware MUST be externally powered,
 * fully booted, correctly leveled and provisioned before running this example.
 * Do not connect UART pins from a host/USB adapter at the wrong voltage.
 * It only starts the modem and prints the IMEI and activation steps. It never
 * transmits. Build it with the CMake target ts_posix_modem.
 */
#define _POSIX_C_SOURCE 200809L
#include "ts_activation.h"
#include "ts_bear9704.h"
#include "ts_posix.h"
#include <stdio.h>
#include <time.h>

static void event(void *u, ts_event_t e, int32_t value)
{
    char text[256];
    ts_bear9704_t *bear = (ts_bear9704_t *)u;
    if (e == TS_EVENT_IMEI && ts_activation_text(ts_bear_imei(bear), 0, text, sizeof text))
        fputs(text, stdout);
    else
        printf("%s detail=%ld\n", ts_event_name(e), (long)value);
}
int main(int argc, char **argv)
{
    ts_posix_t io = {-1, 0};
    ts_bear9704_t sdk;
    ts_platform_t adapter;
    struct timespec sleep_time = {0, 1000000L};
    if (argc != 2) { fprintf(stderr, "Usage: %s /dev/ttyUSB0\n", argv[0]); return 2; }
    if (ts_posix_open(&io, argv[1])) { perror("open UART"); return 1; }
    adapter = ts_posix_platform(&io);
    if (ts_bear_init(&sdk, &adapter, event, &sdk) != TS_OK ||
        ts_bear_start(&sdk) != TS_OK) { ts_posix_close(&io); return 1; }
    for (;;) {
        ts_bear_poll(&sdk);
        nanosleep(&sleep_time, NULL);
    }
}
