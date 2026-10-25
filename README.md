# Telemetry Station SDK

Send small messages from a remote sensor to the cloud over the Iridium satellite network, from any microcontroller, using open-source C firmware and optionally the **BEAR Module 9704**.

## Is this for you?

**A good fit** when your device is somewhere with no cellular or Wi-Fi coverage (ocean, polar, desert, forest, mountains), it sends small readings (bytes to a few kilobytes) from every few minutes to a few times a day, and it runs on a battery or modest power source.

**Not a fit** for continuous or high-rate data (video, audio, frequent large uploads), for devices that need an answer within a second, or for sites that already have reliable cellular coverage, which is usually cheaper.

## See it work in 60 seconds

No hardware needed. You need CMake and a C compiler.

```sh
cd Telemetry_Station_SDK
cmake -S . -B build && cmake --build build && ./build/simulated_demo
```

It runs the real driver against a simulated modem and prints one message's whole life: start, ready, IMEI, send, delivered, shut down.

## Choose your path

| I want to... | Go to |
|---|---|
| Understand the concepts, then try it | [Getting started](Telemetry_Station_SDK/docs/GETTING_STARTED.md) |
| Try it on real Iridium hardware today (Arduino and the 9704 LaunchPad) | [Iridium Development Kit](Iridium_Development_Kit_9704/README.md) |
| Add satellite messaging to my own microcontroller | [SDK](Telemetry_Station_SDK/README.md) and [Porting](Telemetry_Station_SDK/docs/PORTING.md) |
| Review the firmware design | [Design notes](Telemetry_Station_SDK/docs/DESIGN.md) |
| Understand the hardware | [BEAR Module 9704 docs](BEAR_Module/README.md) and [datasheet](BEAR_Module/datasheet/BEAR-9704_Datasheet.md) |
| Evaluate this for my company | [Evaluation guide](docs/EVALUATION.md) |

## How it works

```text
Payload Optimizer --> payload.c / payload.h --> your firmware + this SDK --> BEAR Module 9704 --> Iridium --> cloud
 (design the message)   (encode / decode)        (reads the IMEI)             (satellite modem)
                                                        |
                       calculator --> activate with the IMEI --> telemetrystation.com
```

1. **Design a compact message** with the free [Payload Optimizer](https://telemetrystation.com/payload-optimizer). Download `payload.c` and `payload.h` to encode (and decode) it, and note the **bytes per message**. Smaller messages cost less.
2. **Add this firmware** to your microcontroller and connect your encoder.
3. **Read the modem's IMEI**, its unique 15-digit serial number. The firmware prints it and the next steps.
4. **Size your plan** by entering your needs, including bytes per message, in the [calculator](https://telemetrystation.com/calculators/certus). Then **[activate](https://telemetrystation.com/activate/certus)** the modem with its IMEI. You receive the **topic ID** your messages are sent to.

Using Telemetry Station is encouraged, because the BEAR Module hardware and the data services are designed to work together. It is not required: the driver talks to the modem and works without an account.

## What is the BEAR Module 9704?

The BEAR Module family is a Pacific Gyre product line, built under the Telemetry Station organization, that makes it easier to add satellite connectivity to your own electronics. The **BEAR Module 9704** is the first member, built around an **Iridium 9704** modem. It is in development.

A BEAR Module contains:

- **An Iridium modem.** The 9704 in this first version.
- **Power management.** A satellite modem idles on little power but pulls short, large bursts of current when it transmits. Power management is the circuitry that regulates and stores energy for those bursts and switches the modem on and off in the safe order, so your microcontroller does not have to. From your firmware, it is an `ENABLE` pin and a `BOOTED` pin.
- **An antenna connection**, and in some configurations a qualified Iridium antenna.

## Status and roadmap

| Part | Status | Next |
|---|---|---|
| SDK driver (start-up, send, receive, shutdown, IMEI) | Alpha, version 0.2 | Validation on BEAR Module 9704 hardware, then a 1.0 API freeze |
| MSP430F169 port and example | Reference port | Bench bring-up on the BEAR Module 9704 evaluation setup |
| Arduino development kit | Available | Validation on the Iridium 9704 LaunchPad |
| BEAR Module 9704 hardware | In development | Pinout, electrical limits and availability are published at Rev A. See the [roadmap](BEAR_Module/ROADMAP.md) |
| Certification | Planned | See [Compliance and quality](BEAR_Module/docs/09_Compliance_and_Quality.md) |

The SDK follows [semantic versioning](https://semver.org). While it is below 1.0, the API may change between minor versions. See the [changelog](CHANGELOG.md).

## What is in this repository

| Folder | What it is |
|---|---|
| [`Telemetry_Station_SDK/`](Telemetry_Station_SDK/README.md) | Portable C99 firmware for any microcontroller. Includes a port for the TI MSP430F169, the first test target, a template for your own port, and a computer-based test suite. |
| [`BEAR_Module/`](BEAR_Module/README.md) | The hardware blueprint for the BEAR Module 9704: datasheet, architecture, interfaces, power, RF, compliance, roadmap. |
| [`Iridium_Development_Kit_9704/`](Iridium_Development_Kit_9704/README.md) | An Arduino sketch to try the workflow on the Iridium 9704 LaunchPad and send test messages. |
| [`docs/`](docs/EVALUATION.md) | Guidance for decision makers evaluating the platform. |

Both firmware projects use the same setting names (`TS_TOPIC_ID`, `TS_MAX_PAYLOAD`), the same activation helper, and the same `app_payload` file where you connect your encoder. Code you try in the kit carries over to the SDK.

## Run all the tests

```sh
cd Telemetry_Station_SDK
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure

cd ../Iridium_Development_Kit_9704
sh tests/run_tests.sh && sh tests/compile_adapter.sh
```

## Before you deploy

- Wait for a message to be **delivered**, not just queued.
- The firmware never retries by itself, since a retry after an unknown result could send and bill a message twice. Put a sequence number in your payload.
- Shut the module down properly and wait for `TS_EVENT_STOPPED` before removing its power.
- Add a hardware watchdog, and decide what your product does after a fault.
- Use the topic ID Telemetry Station gives you, never another project's.
- Check voltage levels on both sides before connecting. The MSP430F169 pins in the SDK are test-bench wiring. The BEAR Module 9704 pinout and electrical details are published at Rev A.
- The SDK checks message integrity but does not encrypt or authenticate. See the [security policy](SECURITY.md).

## Help and contributions

Contact hello@telemetrystation.com for questions. Bug reports: open an [issue](https://github.com/Telemetry-Station/sdk/issues). Security reports: see [SECURITY.md](SECURITY.md). Contributing: [CONTRIBUTING.md](CONTRIBUTING.md). Terms used in this project: [glossary](BEAR_Module/GLOSSARY.md).

## License

[MIT](LICENSE). Every source file carries an `SPDX-License-Identifier: MIT` line.

### Third-party software and trademarks

This repository contains no third-party source code. It works with software you install yourself, under its own license:

- **Iridium 9704 LaunchPad library** (used by the Arduino kit): installed from Iridium's package. See its license on the [Iridium documentation site](https://iridium.gitlab.io/9704_launch_pad/documentation/software_setup.html).
- **MSP430 compiler and `msp430.h` device headers** (used by the MSP430F169 port): supplied by your toolchain, for example TI's Code Composer Studio or the MSP430 GCC package, under that toolchain's license.

Iridium and Iridium Certus are trademarks of Iridium Communications Inc. LaunchPad, MSP430 and Code Composer Studio are trademarks of Texas Instruments Incorporated. All other names belong to their owners. They are used here only to describe compatibility, and this project is not affiliated with or endorsed by them. BEAR Module and Telemetry Station are names of the Telemetry Station organization at Pacific Gyre.
