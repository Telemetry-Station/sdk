/* SPDX-License-Identifier: MIT */
#ifndef TS_CONFIG_H
#define TS_CONFIG_H

/*
 * Compile-time configuration. Override any value with -DNAME=value or by
 * defining it before including this header.
 *
 * The defaults fit a small MCU such as the MSP430F169 (2 KB RAM). The driver's
 * static RAM is roughly 2 * TS_LINE_CAP + 2 * TS_MAX_PAYLOAD + 100 bytes
 * (see docs/DESIGN.md for the measured figure).
 */

/* IMT topic your messages are sent to. Telemetry Station provides this when
 * you activate your modem. 0 means "not set yet" and blocks sending. */
#ifndef TS_TOPIC_ID
#define TS_TOPIC_ID 0u
#endif

/* Largest application payload in bytes, before the 2-byte transport CRC.
 * Use the "bytes per message" shown by the Payload Optimizer. */
#ifndef TS_MAX_PAYLOAD
#define TS_MAX_PAYLOAD 64u
#endif

/* Size of the modem command and response line buffers. */
#ifndef TS_LINE_CAP
#define TS_LINE_CAP 256u
#endif

/* Receive (MT) support costs RAM. Turn it off if you only send. */
#ifndef TS_ENABLE_MT
#define TS_ENABLE_MT 1
#endif

/* Ask the modem for its IMEI as soon as it is ready, so the firmware can
 * show it to the user for activation. */
#ifndef TS_QUERY_IMEI_ON_READY
#define TS_QUERY_IMEI_ON_READY 1
#endif

/* Timeouts, in milliseconds. */
#ifndef TS_CMD_TIMEOUT_MS
#define TS_CMD_TIMEOUT_MS 5000ul
#endif
#ifndef TS_BOOT_TIMEOUT_MS
#define TS_BOOT_TIMEOUT_MS 30000ul
#endif
#ifndef TS_MO_TIMEOUT_MS
#define TS_MO_TIMEOUT_MS 600000ul
#endif
#ifndef TS_MT_TIMEOUT_MS
#define TS_MT_TIMEOUT_MS 180000ul
#endif

/* Most bytes ts_bear_poll() reads from the UART per call. */
#ifndef TS_POLL_BYTE_BUDGET
#define TS_POLL_BYTE_BUDGET 64u
#endif

#if TS_MAX_PAYLOAD < 1 || TS_MAX_PAYLOAD > 1024
#error TS_MAX_PAYLOAD must be between 1 and 1024
#endif
#if TS_LINE_CAP < 256 || TS_LINE_CAP > 4096
#error TS_LINE_CAP must be between 256 and 4096
#endif
/* One base64-encoded payload segment plus its JSON command must fit a line. */
#if (4u * ((TS_MAX_PAYLOAD + 4u) / 3u) + 160u) > TS_LINE_CAP
#error TS_LINE_CAP is too small for TS_MAX_PAYLOAD
#endif

#endif
