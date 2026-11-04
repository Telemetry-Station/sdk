/* SPDX-License-Identifier: MIT */
#include "ts_activation.h"


bool ts_imei_is_valid(const char *imei)
{
    size_t i;
    if (!imei) return false;
    for (i = 0; i < 15u; ++i) {
        if (imei[i] < '0' || imei[i] > '9') return false;
    }
    return imei[15] == '\0';
}

/* Bounded append avoids pulling printf into a 2 KiB RAM target. */
static bool append(char *out, size_t capacity, size_t *used, const char *s)
{
    while (*s) {
        if (*used + 1u >= capacity) return false;
        out[(*used)++] = *s++;
    }
    out[*used] = '\0';
    return true;
}
size_t ts_activation_text(const char *imei, unsigned payload_bytes,
                          char *out, size_t capacity)
{
    size_t n=0;
    char digits[3u * sizeof(unsigned) + 1u];
    size_t d=sizeof(digits)-1u;
    if (!out || !capacity || !ts_imei_is_valid(imei)) return 0;
    out[0]='\0'; digits[d]='\0';
#define ADD(text) do { if (!append(out,capacity,&n,(text))) { out[0]='\0'; return 0; } } while (0)
    ADD("Modem IMEI: "); ADD(imei); ADD("\r\n");
    if (payload_bytes) {
        do { digits[--d]=(char)('0'+payload_bytes%10u); payload_bytes/=10u; } while(payload_bytes);
        ADD("1. Size your plan: "); ADD(TS_CALCULATOR_URL); ADD("?payloadBytes="); ADD(digits+d);
        ADD("\r\n2. Activate: "); ADD(TS_ACTIVATE_URL); ADD("\r\n");
    } else {
        ADD("1. Get payload code and bytes per message: "); ADD(TS_OPTIMIZER_URL);
        ADD("\r\n2. Size your plan: "); ADD(TS_CALCULATOR_URL);
        ADD("\r\n3. Activate: "); ADD(TS_ACTIVATE_URL); ADD("\r\n");
    }
#undef ADD
    return n;
}
