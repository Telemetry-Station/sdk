# Telemetry Station: from sensor to insight

Send a small reading from a remote device, then turn it into a value you can use.

**BEAR Module 9704** is the satellite radio hardware. **Telemetry Station SDK** is the software that runs on your microcontroller—the small computer reading your sensor. **Telemetry Station** is the optional service for activation and data delivery.

**[Try your first sample →](Telemetry_Station_SDK/docs/GETTING_STARTED.md#2-see-it-work-with-no-hardware)** No hardware, account or airtime is needed for the computer demo.

## Choose your next step

| Your goal | Start here | You will finish with |
|---|---|---|
| Understand the product and buying decision | [Evaluate the fit](docs/EVALUATION.md) | What exists, what it costs to integrate, and what remains before release |
| See a reading work | [Try the sample](Telemetry_Station_SDK/docs/GETTING_STARTED.md) | A simulated send and a locally decoded temperature |
| Connect a BEAR engineering unit | [Check supported hardware](Telemetry_Station_SDK/docs/GETTING_STARTED.md#3-pick-your-path) | The correct host, wiring and setup procedure |
| Integrate your sensor firmware | [SDK integration](Telemetry_Station_SDK/README.md) | The adapter, sample and checks your product needs |
| Check electrical or enclosure requirements | [BEAR documentation](BEAR_Module/README.md) | The public interface and its validation status |

## Try it on your computer

With Python 3, CMake and C development tools installed, open a terminal in this repository:

```sh
python3 Telemetry_Station_SDK/tools/try_sample.py
```

The command builds in a temporary directory, runs one simulated message and cleans up. Look for **`Local decode preview: sample 0, temperature 21.5 C`** and the completed shutdown. The value is example data. No satellite or cloud service is contacted. [Manual commands and troubleshooting](Telemetry_Station_SDK/docs/GETTING_STARTED.md#2-see-it-work-with-no-hardware).

## The path to a real reading

```text
Your sensor → your microcontroller + SDK → BEAR Module → Iridium → your data destination
```

1. **Try the included sample.** Keep its message format until the connection works.
2. **Connect a supported host and module.** Follow the supplied engineering setup procedure.
3. **Activate the real module.** Its IMEI identifies it; your assigned topic ID identifies the message destination.
4. **Send once and check the result.** Confirm satellite delivery, then confirm and decode the message at your destination.
5. **Add your sensor.** Replace the example reading. Change the message format only if you need different fields.

The SDK handles modem communication and sequencing. Your integration supplies compatible hardware, sensor readings and a configured destination. A delivery event confirms network receipt; it does not prove a dashboard received or decoded the message.

## Status and roadmap

**Available now:** alpha SDK (0.2), computer simulation, sample decoder, public integration documentation and an MSP430F169 engineering reference port.

**Still required for effortless hardware setup:** a complete validated kit, ready-to-load packages for named development boards, guided activation and destination setup, and first-time-user testing. Arduino, ESP32 and STM32 BEAR adapters are not included. The separate [Arduino example](Iridium_Development_Kit_9704/README.md) uses third-party Iridium LaunchPad hardware.

BEAR hardware is in design review. Physical validation, certification and production readiness remain pending. **April 1, 2027 is a tentative sales target, not a confirmed date.** See the [release checklist](BEAR_Module/RD_CHECKLIST.md) before planning a deployment.

Small, occasional readings from remote sites are the intended use. Video, continuous streams and applications needing a guaranteed immediate response are outside this setup's scope.

## What is in this repository

| Folder | Purpose |
|---|---|
| [Telemetry_Station_SDK](Telemetry_Station_SDK/README.md) | Software, examples, setup tools and tests |
| [BEAR_Module](BEAR_Module/README.md) | Public product, interface and verification documentation |
| [Iridium_Development_Kit_9704](Iridium_Development_Kit_9704/README.md) | Separate example for third-party hardware |
| [docs](docs/EVALUATION.md) | Buyer evaluation guide |

## Help and contributions

[Contact Telemetry Station](https://telemetrystation.com/contact) for hardware availability, service and integration questions. [Report a software issue](https://github.com/Telemetry-Station/sdk/issues) with your version, setup and first error. Follow the [security policy](SECURITY.md) for private reports and [contributing guide](CONTRIBUTING.md) for checks before a pull request. Unfamiliar term? Use the [glossary](BEAR_Module/GLOSSARY.md).

## License

[MIT](LICENSE). Every source file carries an `SPDX-License-Identifier: MIT` line.

### Third-party software and trademarks

This repository contains no third-party source code. It works with software you install yourself, under its own license:

- **Iridium 9704 development-board library** (used by the separate Arduino example): installed from Iridium's package. See its license and setup instructions in Iridium's documentation.
- **MSP430 compiler and `msp430.h` device headers** (used by the MSP430F169 port): supplied by your toolchain, for example TI's Code Composer Studio or the MSP430 GCC package, under that toolchain's license.

Iridium and Iridium Certus are trademarks of Iridium Communications Inc. LaunchPad, MSP430 and Code Composer Studio are trademarks of Texas Instruments Incorporated. All other names belong to their owners. They are used here only to describe compatibility, and this project is not affiliated with or endorsed by them. BEAR Module and Telemetry Station are names of Telemetry Station.
