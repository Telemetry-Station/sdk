/* SPDX-License-Identifier: MIT */
#include "ts_internal.h"
#include <ctype.h>
#include <string.h>

/* JSPR is CR-delimited: "GET apiVersion {}\r" and
 * "200 apiVersion {...}\r", "299 messageTerminateSegment {...}\r".
 * The JSON accessors intentionally support the *documented subset* of fields
 * this SDK needs, without malloc or a generic JSON DOM.
 * They enforce top-level property boundaries to avoid accidental substring hits.
 */
static const char *ws(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\n') ++p;
    return p;
}

static const char *skip_string(const char *p)
{
    if (*p++ != '"') return NULL;
    while (*p) {
        if (*p == '"') return p + 1;
        if ((unsigned char)*p < 0x20u) return NULL;
        if (*p == '\\') {
            ++p;
            if (!*p) return NULL;
            if (strchr("\"\\/bfnrtu", *p) == NULL) return NULL;
        }
        ++p;
    }
    return NULL;
}

static const char *skip_value(const char *p)
{
    char close_stack[16];
    unsigned depth = 0;
    p = ws(p);
    if (*p == '"') return skip_string(p);
    if (*p != '{' && *p != '[') {
        if (!*p) return NULL;
        while (*p && *p != ',' && *p != '}' && *p != ']' && *p != '\n') ++p;
        return p;
    }
    close_stack[depth++] = *p++ == '{' ? '}' : ']';
    while (*p && depth) {
        if (*p == '"') {
            p = skip_string(p);
            if (!p) return NULL;
            continue;
        }
        if (*p == '{' || *p == '[') {
            if (depth >= sizeof(close_stack)) return NULL;
            close_stack[depth++] = *p == '{' ? '}' : ']';
        } else if (*p == '}' || *p == ']') {
            if (*p != close_stack[depth - 1]) return NULL;
            --depth;
        }
        ++p;
    }
    return depth == 0 ? p : NULL;
}

static const char *member(const char *json, const char *key)
{
    const char *p = json, *end, *value;
    size_t target_len;
    if (!json || !key || *ws(json) != '{') return NULL;
    target_len = strlen(key);
    p = ws(json) + 1;
    while (*p) {
        const char *start;
        p = ws(p);
        if (*p == '}') return NULL;
        start = p + 1;
        end = skip_string(p);
        if (!end) return NULL;
        p = ws(end);
        if (*p != ':') return NULL;
        value = ws(p + 1);
        if (start <= end - 1 && (size_t)((end - 1) - start) == target_len &&
            memcmp(start, key, target_len) == 0) return value;
        p = skip_value(value);
        if (!p) return NULL;
        p = ws(p);
        if (*p == ',') { ++p; continue; }
        return NULL;
    }
    return NULL;
}

bool ts_json_u32(const char *json, const char *key, uint32_t *value)
{
    const char *p = member(json, key);
    uint32_t n = 0;
    if (!p || !value || *p < '0' || *p > '9') return false;
    do {
        unsigned digit = (unsigned)(*p - '0');
        if (n > (UINT32_MAX - digit) / 10u) return false;
        n = n * 10u + digit;
        ++p;
    } while (*p >= '0' && *p <= '9');
    if (*p != ',' && *p != '}' && *p != ' ' && *p != '\n') return false;
    *value = n;
    return true;
}

bool ts_json_string(const char *json, const char *key, char *out, size_t cap)
{
    const char *p = member(json, key);
    size_t n = 0;
    if (!p || !out || !cap || *p != '"') return false;
    ++p;
    while (*p && *p != '"') {
        if (*p == '\\') {
            /* Fields in use (status, IMEI, base64) should not be escaped. */
            return false;
        }
        if (n + 1 >= cap || (unsigned char)*p < 0x20u) return false;
        out[n++] = *p++;
    }
    if (*p != '"') return false;
    out[n] = 0;
    return true;
}

bool ts_json_string_span(const char *json, const char *key,
                         const char **data, size_t *length)
{
    const char *p = member(json, key), *end;
    if (!p || !data || !length || *p != '"') return false;
    end = skip_string(p);
    if (!end) return false;
    /* Base64 never uses escapes; reject them to preserve exact byte offsets. */
    for (const char *q = p + 1; q < end - 1; ++q)
        if (*q == '\\') return false;
    *data = p + 1;
    *length = (size_t)((end - 1) - (p + 1));
    return true;
}

static bool extract_object(const char *p, char *out, size_t cap)
{
    const char *end;
    size_t n;
    if (!p || *p != '{') return false;
    end = skip_value(p);
    if (!end) return false;
    n = (size_t)(end - p);
    if (n >= cap) return false;
    memcpy(out, p, n);
    out[n] = '\0';
    return true;
}

static bool parse_version_object(const char *p,
                                 uint32_t *major, uint32_t *minor, uint32_t *patch)
{
    char buf[160];
    if (!extract_object(p, buf, sizeof(buf))) return false;
    return ts_json_u32(buf, "major", major) &&
           ts_json_u32(buf, "minor", minor) &&
           ts_json_u32(buf, "patch", patch) &&
           *major <= 255 && *minor <= 255 && *patch <= 255;
}

bool ts_json_version(const char *json, const char *field,
                     uint32_t *major, uint32_t *minor, uint32_t *patch)
{
    return parse_version_object(member(json, field), major, minor, patch);
}

bool ts_json_first_version(const char *json,
                           uint32_t *major, uint32_t *minor, uint32_t *patch)
{
    const char *p = member(json, "supported_versions");
    if (!p || *p != '[') return false;
    p = ws(p + 1);
    return parse_version_object(p, major, minor, patch);
}

bool ts_jspr_parse(char *line, ts_jspr_frame_t *frame)
{
    char *p, *first, *name;
    unsigned c;
    size_t n;
    if (!line || !frame) return false;
    p = line;
    while (*p && ((unsigned char)*p == 0x11u || *p == ' ' || *p == '\n')) ++p;
    if (!isdigit((unsigned char)p[0]) || !isdigit((unsigned char)p[1]) ||
        !isdigit((unsigned char)p[2]) || p[3] != ' ') return false;
    c = (unsigned)(p[0]-'0')*100u + (unsigned)(p[1]-'0')*10u + (unsigned)(p[2]-'0');
    if (c < TS_JSPR_CODE_MIN || c > TS_JSPR_CODE_MAX) return false;
    first = p + 4;
    name = strchr(first, ' ');
    if (!name) return false;
    n = (size_t)(name - first);
    if (!n || n >= sizeof(frame->target)) return false;
    memcpy(frame->target, first, n);
    frame->target[n] = '\0';
    for (n = 0; frame->target[n]; ++n)
        if (!isalnum((unsigned char)frame->target[n])) return false;
    p = name + 1;
    if (*p != '{') return false;
    if (!skip_value(p) || *ws(skip_value(p)) != '\0') return false;
    frame->code = (uint16_t)c;
    frame->json = p;
    return true;
}
