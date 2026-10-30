# Iridium Development Kit 9704

An independent Arduino example for users who already own an Iridium 9704 LaunchPad and want to send messages to [Telemetry Station](https://telemetrystation.com). It uses the official Iridium 9704 LaunchPad library, installed separately under Iridium's own license (it is not included here).

The development kit is third-party hardware, not a Pacific Gyre or Telemetry Station product. This folder contains example software only; it does not imply affiliation with or endorsement by the hardware manufacturer. Refer to the manufacturer for hardware documentation and support.

> **Note:** the sketch sends **demo** coordinates and temperature, not real readings. Sending messages may use credits. Contact [telemetrystation.com/contact](https://telemetrystation.com/contact) for questions.

## The workflow

1. **Get your payload code.** Open the [Payload Optimizer](https://telemetrystation.com/payload-optimizer), describe your data, and download `payload.c` and `payload.h`. The tool tells you your **bytes per message**. This kit includes a sample pair in `src/generated/` so you can try it first.
2. **Set your size.** Put the bytes per message in `TS_MAX_PAYLOAD` in [`src/ts_config.h`](src/ts_config.h).
3. **Find your IMEI.** Upload the sketch, open the Serial Monitor at 115200 baud, and read the 15-digit modem IMEI from the startup log (or the modem's label). Type `i <IMEI>` to get your next steps with the right links.
4. **Size your plan and activate.** Use the [calculator](https://telemetrystation.com/calculators/certus), then [activate](https://telemetrystation.com/activate/certus) with your IMEI. Telemetry Station gives you a **topic ID**.
5. **Send.** Put the topic in `TS_TOPIC_ID`, upload again, and type `s` (or press the USER button).

## Setup

1. Install Iridium's [Satellite Boards package and 9704 LaunchPad library](https://iridium.gitlab.io/9704_launch_pad/documentation/software_setup.html) and select the **Iridium 9704 Launch Pad**.
2. Open `Iridium_Development_Kit_9704.ino` in the Arduino IDE.
3. Connect the **battery** and antenna. USB power alone is not enough to transmit. Go outside with a clear view of the sky.

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

The sample in `src/generated/` was exported for the SBD network, not Certus. Regenerate it for a real 9704 deployment.

Two layers are kept apart. The generated code packs only your data. The kit adds a 2-byte CRC around it before sending. When decoding in the Payload Optimizer, use the payload bytes without those 2 bytes. The Serial Monitor prints the full message as hex; remove its last 2 bytes first.

## Tests

From this folder:

```sh
sh tests/run_tests.sh      # payload golden vector and logic tests
sh tests/compile_adapter.sh   # Arduino code compiles against a stub library
```

The tests check the logic and that the Arduino code compiles against a stub of Iridium's library. They do not verify operation on physical hardware.
