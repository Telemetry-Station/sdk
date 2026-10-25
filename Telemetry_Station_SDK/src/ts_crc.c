/* SPDX-License-Identifier: MIT */
#include "ts_internal.h"
/* CRC-16/CCITT-FALSE polynomial 0x1021, initial remainder 0x0000.
 * Iridium IMT host-side application message adds two bytes, big-endian.
 * Sensor payload CRCs (if any) are a separate protocol concern.
 */
uint16_t ts_crc16(const uint8_t *p, size_t n)
{
    uint16_t crc = 0;
    size_t i;
    if (!p && n) return 0;
    for (i = 0; i < n; ++i) {
        uint8_t bit;
        crc ^= (uint16_t)((uint16_t)p[i] << 8);
        for (bit = 0; bit < 8; ++bit)
            if (crc & 0x8000u)
                crc = (uint16_t)(((uint32_t)crc << 1u) ^ (uint32_t)0x1021u);
            else
                crc = (uint16_t)((uint32_t)crc << 1u);
    }
    return crc;
}
