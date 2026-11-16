# Iridium Development Kit 9704

An independent Arduino example for users who already own an Iridium 9704 LaunchPad and want to send messages to [Telemetry Station](https://telemetrystation.com). It uses the official Iridium 9704 LaunchPad library, installed separately under Iridium's own license (it is not included here).

The development kit is third-party hardware, not a Telemetry Station product. This folder contains example software only; it does not imply affiliation with or endorsement by the hardware manufacturer. Refer to the manufacturer for hardware documentation and support.

> **Note:** the sketch sends **demo** coordinates and temperature, not real readings. Sending messages may use credits. Contact [telemetrystation.com/contact](https://telemetrystation.com/contact) for questions.

## Start with the hardware you actually own

This guide is only for the third-party **Iridium 9704 LaunchPad**. It is not an Arduino adapter for BEAR. **Using BEAR? [Follow the BEAR setup guide →](../Telemetry_Station_SDK/docs/GETTING_STARTED.md)**

## Send the included example once

1. Follow Iridium’s [board and library installation instructions](https://iridium.gitlab.io/9704_launch_pad/documentation/software_setup.html), then select **Iridium 9704 Launch Pad** in the Arduino IDE.
2. Open `Iridium_Development_Kit_9704.ino`. Keep `TS_TOPIC_ID` at zero initially. Keep the included demo payload and buffer size for this first checkout.
3. Connect the battery and antenna according to the manufacturer’s procedure. USB power alone is not enough to transmit. Use a clear sky view for a live test.
4. Upload, then open Serial Monitor at **115200 baud**. Read the actual 15-digit IMEI from the log or module label. Type `i <IMEI>` for activation instructions.
5. Confirm sample message sizing and service destination with the provider, activate the actual modem, and put your assigned topic in `TS_TOPIC_ID` in [`src/ts_config.h`](src/ts_config.h). Upload again. Keep assigned identifiers out of public commits.
6. Type **`s`** once (or press USER). Look for `ACCEPTED`, then `DELIVERED`. Confirm the same message appears at your configured service destination.

**Success:** the message is delivered and its example values decode correctly at the destination. The sketch uses synthetic coordinates and temperature; it is not connected to your sensors. Setup and live hardware operation have not been validated by the host tests.

## Serial Monitor commands (115200 baud)

| Command    | What it does                                                |
| ---------- | ----------------------------------------------------------- |
| `s`        | Encode and send **one** demo message                        |
| `i`        | Explain how to find your IMEI                               |
| `i <IMEI>` | Show your calculator and activation steps for that IMEI     |
| `d`        | Show status: signal, battery, topic, send state             |
| `u`        | Release an **unconfirmed** request. A duplicate may result. |
| `?`        | Help                                                        |

After sending, look for `ACCEPTED` (the modem queued it) and then `DELIVERED` (Iridium confirmed receipt). Accepted alone does not mean delivered. The kit never retries on its own, because a retry after an unknown result could send and bill the message twice.

## How it is organized

```text
Iridium_Development_Kit_9704.ino   Arduino entry point
src/
  ts_config.h          Settings: TS_TOPIC_ID, TS_MAX_PAYLOAD
  app_payload.c/.h     YOU EDIT: maps your readings to the generated encoder
  generated/           Payload Optimizer output (payload .c and .h)
  ts_mo_tracker.c/.h   Tracks the one message being sent (no hardware)
  ts_activation.c/.h   IMEI check and activation steps (same file as the SDK)
  ts_launchpad.cpp/.h  The only Arduino and Iridium-library code
schema/                Decoder schema for the sample payload
tests/                 Tests that run on a computer
```

The example uses `TS_TOPIC_ID` for the Telemetry Station topic, `TS_MAX_PAYLOAD` for the message size, and `app_payload` to connect application readings to the encoder.

## Using your own payload

1. Download `payload.c` and `payload.h` from the Payload Optimizer. Use the SDK export, not the stand-alone download, which has its own `main()`. Choose the **Certus** network for a 9704.
2. Replace `src/generated/telemetry_payload.c` and `.h` with them, keeping those file names. Save the matching schema JSON in `schema/`.
3. Edit `app_payload.h` and `app_payload.c` so the reading struct and the `values.field = ...` lines match the new generated header.
4. Set `TS_MAX_PAYLOAD` to your bytes per message. The build stops with an error if the generated payload can be larger.
5. Run the tests (below).

The included demonstration codec was exported for SBD, not Certus. Confirm that demonstration format with your receiver; regenerate for Certus and validate the matching decoder before using this in a real 9704 deployment. Do not use the BEAR sequence/temperature decoder for this different sample schema.

Two layers are kept apart. The generated code packs only your data. The kit adds a 2-byte CRC around it before sending. When decoding in the Payload Optimizer, use the payload bytes without those 2 bytes. The Serial Monitor prints the full message as hex; remove its last 2 bytes first.

## Tests

From this folder:

```sh
sh tests/run_tests.sh      # payload golden vector and logic tests
sh tests/compile_adapter.sh   # Arduino code compiles against a stub library
```

The tests check the logic and that the Arduino code compiles against a stub of Iridium's library. They do not verify operation on physical hardware.

**Next:** after the sample is received, [replace the demo fields](#using-your-own-payload), update the receiver decoder and compare a real sensor reading with the received value. Hardware setup and support remain with the kit manufacturer.
