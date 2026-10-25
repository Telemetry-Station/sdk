/* SPDX-License-Identifier: MIT */
#ifndef TS_ACTIVATION_H
#define TS_ACTIVATION_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Telemetry Station pages used in the setup flow. */
#define TS_OPTIMIZER_URL  "https://telemetrystation.com/payload-optimizer"
#define TS_CALCULATOR_URL "https://telemetrystation.com/calculators/certus"
#define TS_ACTIVATE_URL   "https://telemetrystation.com/activate/certus"

/* True if the text is exactly 15 digits. The check digit is not enforced. */
bool ts_imei_is_valid(const char *imei);

/*
 * Writes a short, printable guide that shows the modem IMEI and the next
 * steps (calculator, then activation). Pass payload_bytes = 0 if unknown.
 * Returns the length written, or 0 if the IMEI is invalid or out does not fit.
 */
size_t ts_activation_text(const char *imei, unsigned payload_bytes,
                          char *out, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
