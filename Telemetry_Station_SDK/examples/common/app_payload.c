/* SPDX-License-Identifier: MIT */
#include "app_payload.h"

/*
 * Placeholder encoding: 2 bytes of sequence, 2 bytes of temperature.
 * Replace the body with the Payload Optimizer's generated encoder:
 *
 *   #include "telemetry_payload.h"
 *
 *   telemetry_payload_values_t values = {0};
 *   values.message_sequence = reading->sequence;          // use your field names
 *   values.temperature_tenths_c = reading->temperature_tenths_c;
 *   return telemetry_payload_encode(&values, out, capacity, bytes, bits);
 */
bool app_payload_encode(const app_reading_t *reading, uint8_t *out,
                        size_t capacity, size_t *bytes, size_t *bits)
{
    if (!reading || !out || !bytes || !bits || capacity < 4u) return false;
    out[0] = (uint8_t)(reading->sequence >> 8);
    out[1] = (uint8_t)(reading->sequence & 0xffu);
    out[2] = (uint8_t)((uint16_t)reading->temperature_tenths_c >> 8);
    out[3] = (uint8_t)((uint16_t)reading->temperature_tenths_c & 0xffu);
    *bytes = 4u;
    *bits = 32u;
    return true;
}
