/* SPDX-License-Identifier: MIT */
#include "app_payload.h"
#include "generated/telemetry_payload.h"
#include "ts_config.h"
#if TELEMETRY_PAYLOAD_MAX_BYTES > TS_MAX_PAYLOAD
#error Optimizer schema exceeds the configured transport buffer
#endif
bool app_payload_encode(const app_reading_t *reading, uint8_t *out,
                        size_t capacity, size_t *bytes, size_t *bits)
{
    telemetry_payload_values_t values = {0};
    if (!reading) return false;
    values.sequence = reading->sequence;
    values.temperature_tenths_c = reading->temperature_tenths_c;
    return telemetry_payload_encode(&values, out, capacity, bytes, bits);
}
