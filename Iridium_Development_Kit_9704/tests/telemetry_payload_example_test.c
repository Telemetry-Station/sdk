/* Generated golden-vector test. Compile WITH telemetry_payload.c. */
#include "telemetry_payload.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    telemetry_payload_values_t values = {0};
    values.latitude = 32.87051;
    values.longitude = -117.24103;
    values.message_sequence = UINT64_C(37);
    values.temperature_tenths_c = -INT64_C(27);
    values.gps_fix = true;
    char device_id_source[] = { 0x42, 0x55, 0x4f, 0x59, 0x2d, 0x30, 0x31, 0x00 };
    values.device_id = device_id_source;
    values.device_id_length = 7u;
    uint8_t diagnostic_bytes_source[] = { 0x01, 0xff, 0x10 };
    values.diagnostic_bytes = diagnostic_bytes_source;
    values.diagnostic_bytes_length = 3u;
    uint8_t encoded[21u] = {0};
    size_t bytes = 0u, bits = 0u;
    assert(telemetry_payload_encode(&values, encoded, sizeof(encoded), &bytes, &bits));
    assert(bytes == 21u && bits == 168u);
    const char *expected = "5dbe258bf867204bffcb742554f592d3031301ff10";
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < bytes; ++i) {
        assert(hex[encoded[i] >> 4] == expected[i * 2u]);
        assert(hex[encoded[i] & 0x0fu] == expected[i * 2u + 1u]);
    }
#if TELEMETRY_PAYLOAD_ENABLE_DECODE
    telemetry_payload_values_t decoded = {0};
    char device_id_result[13u] = {0};
    decoded.device_id = device_id_result;
    decoded.device_id_capacity = sizeof(device_id_result);
    uint8_t diagnostic_bytes_result[8u] = {0};
    decoded.diagnostic_bytes = diagnostic_bytes_result;
    decoded.diagnostic_bytes_capacity = sizeof(diagnostic_bytes_result);
    assert(telemetry_payload_decode(encoded, bytes, &decoded));
    assert(fabs(decoded.latitude - values.latitude) <= 0.0000051);
    assert(fabs(decoded.longitude - values.longitude) <= 0.0000051);
    assert(decoded.message_sequence == values.message_sequence);
    assert(decoded.temperature_tenths_c == values.temperature_tenths_c);
    assert(decoded.gps_fix == values.gps_fix);
    assert(decoded.device_id_length == values.device_id_length);
    assert(memcmp(decoded.device_id, values.device_id, values.device_id_length) == 0);
    assert(decoded.diagnostic_bytes_length == values.diagnostic_bytes_length);
    assert(memcmp(decoded.diagnostic_bytes, values.diagnostic_bytes, values.diagnostic_bytes_length) == 0);
#endif
    puts("Telemetry Station SDK payload golden vector: OK");
    return 0;
}
