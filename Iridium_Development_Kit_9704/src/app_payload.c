/* SPDX-License-Identifier: MIT */
#include "app_payload.h"

#include <string.h>

#include "generated/telemetry_payload.h"
#include "ts_config.h"

#if TELEMETRY_PAYLOAD_MAX_BYTES > TS_MAX_PAYLOAD
#error "The generated payload can exceed TS_MAX_PAYLOAD. Raise TS_MAX_PAYLOAD or shrink the schema."
#endif

app_reading_t app_demo_reading(uint32_t sequence, int battery_mv, int signal_bars)
{
    app_reading_t reading;
    memset(&reading, 0, sizeof reading);

    reading.sequence = sequence % 4096u;
    reading.latitude = 32.87051;
    reading.longitude = -117.24103;
    reading.temperature_tenths_c = -27;
    reading.gps_fix = false; /* No GPS is read in this demo. */
    strcpy(reading.device_id, "DEMO-9704");

    if (battery_mv < 0) battery_mv = 0;
    if (battery_mv > 65535) battery_mv = 65535;
    if (signal_bars < 0) signal_bars = 0;
    if (signal_bars > 255) signal_bars = 255;
    reading.diagnostics[0] = (uint8_t)(battery_mv >> 8);
    reading.diagnostics[1] = (uint8_t)(battery_mv & 0xff);
    reading.diagnostics[2] = (uint8_t)signal_bars;
    reading.diagnostics_length = 3;
    return reading;
}

bool app_payload_encode(const app_reading_t *reading, uint8_t *out,
                        size_t capacity, size_t *bytes, size_t *bits)
{
    /* The generated struct takes writable pointers, so encode from copies. */
    char device_id[sizeof reading->device_id];
    uint8_t diagnostics[sizeof reading->diagnostics];
    telemetry_payload_values_t values;
    size_t id_length = 0;

    if (!reading || !out || !bytes || !bits) return false;
    if (reading->diagnostics_length > sizeof diagnostics) return false;

    memcpy(device_id, reading->device_id, sizeof device_id);
    memcpy(diagnostics, reading->diagnostics, sizeof diagnostics);
    while (id_length < sizeof device_id && device_id[id_length]) ++id_length;
    if (id_length == sizeof device_id) return false; /* Missing terminator. */

    memset(&values, 0, sizeof values);
    /* Edit these assignments if you change the field names in the optimizer. */
    values.latitude = reading->latitude;
    values.longitude = reading->longitude;
    values.message_sequence = reading->sequence;
    values.temperature_tenths_c = reading->temperature_tenths_c;
    values.gps_fix = reading->gps_fix;
    values.device_id = device_id;
    values.device_id_length = id_length;
    values.diagnostic_bytes = diagnostics;
    values.diagnostic_bytes_length = reading->diagnostics_length;

    return telemetry_payload_encode(&values, out, capacity, bytes, bits);
}

const char *app_payload_schema_id(void)
{
    return TELEMETRY_PAYLOAD_SCHEMA_ID;
}
