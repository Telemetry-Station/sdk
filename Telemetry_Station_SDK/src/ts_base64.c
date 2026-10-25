/* SPDX-License-Identifier: MIT */
#include "ts_internal.h"
static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t ts_base64_encode(const uint8_t *in, size_t len, char *out, size_t cap)
{
    size_t i = 0, o = 0;
    if ((!in && len) || !out || cap < 1 || len > (cap / 4u) * 3u) return 0;
    while (i < len) {
        size_t rem = len - i;
        uint32_t n = (uint32_t)in[i++] << 16;
        if (rem > 1) n |= (uint32_t)in[i++] << 8;
        if (rem > 2) n |= in[i++];
        if (o + 4 >= cap) return 0;
        out[o++] = alphabet[(n >> 18) & 63u];
        out[o++] = alphabet[(n >> 12) & 63u];
        out[o++] = rem > 1 ? alphabet[(n >> 6) & 63u] : '=';
        out[o++] = rem > 2 ? alphabet[n & 63u] : '=';
    }
    out[o] = '\0';
    return o;
}

static int b64val(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

bool ts_base64_decode(const char *in, size_t len, uint8_t *out,
                      size_t cap, size_t *written)
{
    size_t i, n = 0;
    if (written) *written = 0;
    if (!in || !out || !written || len % 4u) return false;
    for (i = 0; i < len; i += 4u) {
        int a = b64val(in[i]), b = b64val(in[i+1]);
        int c = in[i+2] == '=' ? 0 : b64val(in[i+2]);
        int d = in[i+3] == '=' ? 0 : b64val(in[i+3]);
        bool last = (i + 4 == len);
        if (a < 0 || b < 0 || c < 0 || d < 0) return false;
        if ((in[i+2] == '=' && in[i+3] != '=') ||
            (!last && (in[i+2] == '=' || in[i+3] == '='))) return false;
        if ((in[i+2] == '=' && (b & 15)) ||
            (in[i+3] == '=' && in[i+2] != '=' && (c & 3))) return false;
        if (n >= cap) return false;
        out[n++] = (uint8_t)((a << 2) | (b >> 4));
        if (in[i+2] != '=') {
            if (n >= cap) return false;
            out[n++] = (uint8_t)((b << 4) | (c >> 2));
        }
        if (in[i+3] != '=') {
            if (n >= cap) return false;
            out[n++] = (uint8_t)((c << 6) | d);
        }
    }
    *written = n;
    return true;
}
