/* Generated golden-vector test. Compile WITH telemetry_payload.c. */
#include "telemetry_payload.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    telemetry_payload_values_t values = {0};
    values.sequence = UINT64_C(4660);
    values.temperature_tenths_c = -INT64_C(123);
    uint8_t encoded[4u] = {0};
    size_t bytes = 0u, bits = 0u;
    assert(telemetry_payload_encode(&values, encoded, sizeof(encoded), &bytes, &bits));
    assert(bytes == 4u && bits == 32u);
    const char *expected = "1234ff85";
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < bytes; ++i) {
        assert(hex[encoded[i] >> 4] == expected[i * 2u]);
        assert(hex[encoded[i] & 0x0fu] == expected[i * 2u + 1u]);
    }
#if TELEMETRY_PAYLOAD_ENABLE_DECODE
    telemetry_payload_values_t decoded = {0};
    assert(telemetry_payload_decode(encoded, bytes, &decoded));
    assert(decoded.sequence == values.sequence);
    assert(decoded.temperature_tenths_c == values.temperature_tenths_c);
#endif
    puts("Telemetry Station SDK payload golden vector: OK");
    return 0;
}
