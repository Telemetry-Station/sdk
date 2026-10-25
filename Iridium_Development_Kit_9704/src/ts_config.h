/* SPDX-License-Identifier: MIT */
#ifndef TS_CONFIG_H
#define TS_CONFIG_H

/*
 * Settings for this kit. The names match the Telemetry Station SDK, so the
 * same values carry over when you move to your own microcontroller.
 */

/* Your IMT topic from Telemetry Station (64 or higher). Get it by activating
 * the modem with the IMEI this sketch prints (type 'i' in the Serial Monitor).
 * 0 means "not set yet" and blocks sending. */
#ifndef TS_TOPIC_ID
#define TS_TOPIC_ID 0
#endif

/* Largest payload in bytes, before the 2-byte transport CRC. Use the
 * "bytes per message" shown by the Payload Optimizer. The LaunchPad has more
 * RAM than a small MCU, so the default is larger than the SDK's. */
#ifndef TS_MAX_PAYLOAD
#define TS_MAX_PAYLOAD 190
#endif

/* Milliseconds to wait for the modem to accept a request before warning. */
#ifndef TS_ACCEPT_TIMEOUT_MS
#define TS_ACCEPT_TIMEOUT_MS 60000UL
#endif

#endif
