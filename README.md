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

| I want to...                                      | Go to                                                                                                        |
| ------------------------------------------------- | ------------------------------------------------------------------------------------------------------------ |
| Understand the concepts, then try it              | [Getting started](Telemetry_Station_SDK/docs/GETTING_STARTED.md)                                             |
| Send to Telemetry Station from an Iridium 9704 LaunchPad I already own | [Independent Arduino example](Iridium_Development_Kit_9704/README.md) |
| Add satellite messaging to my own microcontroller | [SDK](Telemetry_Station_SDK/README.md) and [Porting](Telemetry_Station_SDK/docs/PORTING.md)                  |
| Review the firmware design                        | [Design notes](Telemetry_Station_SDK/docs/DESIGN.md)                                                         |
| Understand the hardware                           | [BEAR Module 9704 docs](BEAR_Module/README.md) and [datasheet](BEAR_Module/datasheet/BEAR-9704_Datasheet.md) |
| Evaluate this for my company                      | [Evaluation guide](docs/EVALUATION.md)                                                                       |

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

The BEAR Module family is a Pacific Gyre product line, built under the Telemetry Station organization, that makes it easier to add satellite connectivity to your own electronics. The **BEAR Module 9704** is the first member, built around an **Iridium 9704** modem. It is being designed as a host-independent, production-qualified satellite communications module; the design is in development and is not yet production-qualified.

The customer's host MCU is external to the module. The TI MSP430F169 is the first SDK reference host, not a requirement for using BEAR. The interface is intended for STM32, MSP430, Nordic, Microchip, ESP32 and other microcontrollers, with compatibility validated per MCU and SDK port.

A BEAR Module contains:

- **An Iridium modem.** The 9704 in this first version.
- **Power management.** A satellite modem idles on little power but pulls short, large bursts of current when it transmits. Power management is the circuitry that regulates and stores energy for those bursts and switches the modem on and off in the safe order, so your microcontroller does not have to. From your firmware, it is an `ENABLE` pin and a `BOOTED` pin.
- **An antenna connection**, and in some configurations a qualified Iridium antenna.

## Status and roadmap

The [BEAR Module R&D and release checklist](BEAR_Module/RD_CHECKLIST.md) summarizes current product status and remaining release milestones. **Target sales availability is April 1, 2027**, subject to validation, approvals, and production readiness; this is not a confirmed release date.

| Part                                                 | Status                                          | Next                                                                                                                                       |
| ---------------------------------------------------- | ----------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------ |
| SDK driver (start-up, send, receive, shutdown, IMEI) | Alpha, version 0.2                              | Validation on BEAR Module 9704 hardware, then a 1.0 API freeze                                                                             |
| MSP430F169 port and example                          | First reference host, not a product requirement | SDK validation on BEAR engineering units at G2                                                                 |
| Arduino, STM32 and Raspberry Pi quickstarts          | Planned                                         | Tested setup and integration instructions per platform                                                                             |
| Sample decoders                                      | Planned                                         | Runnable decoder examples with matching test vectors                                                                               |
| Public status page and SLA                           | Planned                                         | Status URL, service scope and approved service-level terms available before general availability                                             |
| Evaluation-kit availability                          | Planned                                         | Confirmed dates available after hardware, validation and supply readiness are established                                                           |
| BEAR Module 9704 hardware                            | In development                                  | Pinout, electrical limits and availability details **[Pending decision]** for the Rev A release. See the [roadmap](BEAR_Module/ROADMAP.md) |
| Certification                                        | Planned                                         | See [Compliance and quality](BEAR_Module/docs/09_Compliance_and_Quality.md)                                                                |

The SDK follows [semantic versioning](https://semver.org). While it is below 1.0, the API may change between minor versions. See the [changelog](CHANGELOG.md).

## What is in this repository

| Folder                                                                    | What it is                                                                                                                                                                  |
| ------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [`Telemetry_Station_SDK/`](Telemetry_Station_SDK/README.md)               | Portable C99 firmware for any microcontroller. Includes a port for the TI MSP430F169, the first test target, a template for your own port, and a computer-based test suite. |
| [`BEAR_Module/`](BEAR_Module/README.md)                                   | The hardware blueprint for the BEAR Module 9704: datasheet, architecture, interfaces, power, RF, compliance, roadmap.                                                       |
| [`Iridium_Development_Kit_9704/`](Iridium_Development_Kit_9704/README.md) | Separate Arduino example for owners of third-party Iridium 9704 LaunchPad hardware to send messages to Telemetry Station. |
| [`docs/`](docs/EVALUATION.md)                                             | Guidance for decision makers evaluating the platform.                                                                                                                       |

The Iridium Development Kit is third-party hardware, not a Pacific Gyre or Telemetry Station product. The Arduino example in this repository is separate from the BEAR Module product, development roadmap, and qualification. It does not require a BEAR Module. Hardware setup and support remain with the hardware manufacturer; the example documents sending messages to Telemetry Station and does not imply affiliation or endorsement.

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
- Check voltage levels on both sides before connecting. The MSP430F169 pins in the SDK are test-bench wiring. BEAR Module 9704 electrical limits, pin numbering and connector details are **[Pending decision]** and will be frozen for the Rev A datasheet.
- The SDK checks message integrity but does not encrypt or authenticate. See the [security policy](SECURITY.md).

## Help and contributions

Contact [telemetrystation.com/contact](https://telemetrystation.com/contact) for questions. Bug reports: open an [issue](https://github.com/Telemetry-Station/sdk/issues). Security reports: see [SECURITY.md](SECURITY.md). Contributing: [CONTRIBUTING.md](CONTRIBUTING.md). Terms used in this project: [glossary](BEAR_Module/GLOSSARY.md).

## License

[MIT](LICENSE). Every source file carries an `SPDX-License-Identifier: MIT` line.

### Third-party software and trademarks

This repository contains no third-party source code. It works with software you install yourself, under its own license:

- **Iridium 9704 development-board library** (used by the separate Arduino example): installed from Iridium's package. See its license and setup instructions in Iridium's documentation.
- **MSP430 compiler and `msp430.h` device headers** (used by the MSP430F169 port): supplied by your toolchain, for example TI's Code Composer Studio or the MSP430 GCC package, under that toolchain's license.

Iridium and Iridium Certus are trademarks of Iridium Communications Inc. LaunchPad, MSP430 and Code Composer Studio are trademarks of Texas Instruments Incorporated. All other names belong to their owners. They are used here only to describe compatibility, and this project is not affiliated with or endorsed by them. BEAR Module and Telemetry Station are names of the Telemetry Station organization at Pacific Gyre.
