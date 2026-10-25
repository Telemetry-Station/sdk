/* SPDX-License-Identifier: MIT */
#include "ts_activation.h"

#include <stdio.h>

bool ts_imei_is_valid(const char *imei)
{
    size_t i;
    if (!imei) return false;
    for (i = 0; i < 15u; ++i) {
        if (imei[i] < '0' || imei[i] > '9') return false;
    }
    return imei[15] == '\0';
}

size_t ts_activation_text(const char *imei, unsigned payload_bytes,
                          char *out, size_t capacity)
{
    int n;
    if (!out || !capacity || !ts_imei_is_valid(imei)) return 0;

    if (payload_bytes) {
        n = snprintf(out, capacity,
                     "Modem IMEI: %s\r\n"
                     "1. Size your plan: %s?payloadBytes=%u\r\n"
                     "2. Activate: %s\r\n",
                     imei, TS_CALCULATOR_URL, payload_bytes, TS_ACTIVATE_URL);
    } else {
        n = snprintf(out, capacity,
                     "Modem IMEI: %s\r\n"
                     "1. Get payload code and bytes per message: %s\r\n"
                     "2. Size your plan: %s\r\n"
                     "3. Activate: %s\r\n",
                     imei, TS_OPTIMIZER_URL, TS_CALCULATOR_URL, TS_ACTIVATE_URL);
    }
    if (n < 0 || (size_t)n >= capacity) {
        out[0] = '\0';
        return 0;
    }
    return (size_t)n;
}
