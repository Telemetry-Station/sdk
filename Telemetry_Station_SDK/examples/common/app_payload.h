/* SPDX-License-Identifier: MIT */
#ifndef APP_PAYLOAD_H
#define APP_PAYLOAD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * The one file you change to match your device. It turns your sensor values
 * into the bytes you send. Keep the same signature after switching to the
 * Payload Optimizer's generated code.
 */
typedef struct {
    uint16_t sequence;            /* Increases with every message. */
    int16_t temperature_tenths_c; /* -27 means -2.7 C. */
} app_reading_t;

/* Writes the message into out and returns false if it does not fit. */
bool app_payload_encode(const app_reading_t *reading, uint8_t *out,
                        size_t capacity, size_t *bytes, size_t *bits);

#endif
