# Evaluation guide

For technical leaders deciding whether to build satellite connectivity into a product on this platform. It covers what exists today, what is on the roadmap, what you would depend on, and how to evaluate it at low cost.

Contact [telemetrystation.com/contact](https://telemetrystation.com/contact) for questions.

## The short version

|                    |                                                                                                                                                                          |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **What it is**     | An open-source firmware SDK, a satellite modem module (the BEAR Module 9704, in development) and an optional data service (Telemetry Station), designed to work together |
| **What it does**   | Sends small messages from remote devices to the cloud over the Iridium network, with a path from message design to activation to billing                                 |
| **Maturity**       | SDK in alpha (version 0.2). Hardware in development, with a published [roadmap](../BEAR_Module/ROADMAP.md)                                                               |
| **Openness**       | The firmware is MIT-licensed and works without a Telemetry Station account. The hardware and the service are designed as one workflow                                    |
| **Best use today** | Evaluation and prototyping, ahead of production supply                                                                                                                   |

## Where each part is

| Area                         | Today                                                                                                                      | Next milestone                                                                                                         |
| ---------------------------- | -------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| Firmware driver              | Alpha. Complete start-up, send, receive, IMEI and shutdown flow, with a computer-based test suite                          | Validation on BEAR Module 9704 engineering units, then a 1.0 API freeze                                                |
| Host MCU                     | MSP430F169 is the first SDK reference host                                                                                 | Compatibility is intended for STM32, MSP430, Nordic, Microchip, ESP32 and other MCUs; ports are validated individually |
| Hardware module              | Interfaces and architecture defined                                                                                        | Specifications published at Rev A, then engineering builds and validation                                              |
| Regulatory approvals         | Planned for each target market                                                                                             | Certification testing, with certificates published as issued                                                           |
| Production supply            | Roadmap                                                                                                                    | Production readiness (see the [roadmap](../BEAR_Module/ROADMAP.md))                                                    |
| Service terms and SLA        | **[Pending decision]**                                                                                                     | Public status page, service terms and SLA before general availability                                                  |
| Customer launch resources    | SDK quickstarts, decoder samples and changelog                                                                             | Arduino, STM32 and Raspberry Pi quickstarts and sample decoders planned; changelog available                           |
| Evaluation-kit dates         | **[Pending decision]**                                                                                                     | Firm public dates after build, validation and supply readiness                                                         |

## What a decision maker usually asks

### Who is behind this, and will it be maintained?

The Telemetry Station organization at Pacific Gyre. The repository is public, so you can see the history and open issues and read every line. A support and lifecycle policy (product change notices, long-term availability) is **[Pending decision]** and will be published in [Ordering and support](../BEAR_Module/docs/11_Ordering_and_Support.md).

### What am I locked into?

- **Firmware:** MIT license. You can fork it. The driver talks to the modem's standard serial protocol and works without an account.
- **Modem:** The BEAR Module uses an Iridium 9704, so it operates on the Iridium network.
- **Service:** Telemetry Station is the path we optimise for, including message design, sizing and activation. The driver does not require it.
- **Message format:** The Payload Optimizer generates plain C encode and decode code that you own and can read.

Data delivery and export options are **[Pending decision]**. Supported formats, retention, and export behavior will be documented before service launch.

### Does it work anywhere?

Iridium operates a low-earth-orbit satellite constellation with global coverage, including the poles and oceans, which makes it well suited to remote sites. Message latency depends on the network and your sky view. BEAR-specific end-to-end latency measurements **[Pending decision]** will be collected during hardware validation; no delivery-time guarantee is stated here.

### How secure is it?

- **Integrity:** every message carries a CRC-16 that detects corruption.
- **Payload protection:** the SDK does not encrypt or authenticate messages. If your data is sensitive, protect it inside your payload. Additional payload-security guidance **[Pending decision]** will be published with the SDK.
- **Firmware surface:** the driver does not use dynamic memory, bounds every buffer and rejects malformed lines. Its tests run under AddressSanitizer, and CI runs static analysis. Fuzz testing and an external review are on the roadmap.
- **Reporting:** vulnerabilities go through the [security policy](../SECURITY.md).

### What will it cost?

Three parts: the module (unit price **[Pending decision]**), the airtime and service (use the [calculator](https://telemetrystation.com/calculators/certus) with your message size and frequency), and your integration engineering. The Payload Optimizer reduces the second part by shrinking messages.

### What does it take to integrate?

For an engineer who knows embedded C: a day to run the simulated demo and read the design notes, a few days to write a port and wire a module, then field testing outdoors. There are no dependencies beyond a C99 compiler. The driver needs about 0.8 KB of RAM (measured on a computer). Flash and stack measurements **[Pending decision]** will be published for each validated target. See [Design notes](../Telemetry_Station_SDK/docs/DESIGN.md).

### Will it scale to a fleet?

That is the design goal. Every module is intended to carry a serial number and IMEI label, with factory test records linked to both, and activation is per IMEI. Fleet provisioning tools and volume terms are **[Pending decision]**. Production quality targets are in [Compliance and quality](../BEAR_Module/docs/09_Compliance_and_Quality.md).

### Is it certified?

Not yet. Iridium and regulatory approvals are planned at gate G4. The markets under assessment are listed in [Compliance and quality](../BEAR_Module/docs/09_Compliance_and_Quality.md); the applicable standards and approval scope for each market and configuration are **[Pending decision]** and are confirmed with the accredited test laboratory. RoHS and REACH are design requirements. Certificates are published as they are issued, and the [R&D and release checklist](../BEAR_Module/RD_CHECKLIST.md) shows current status. Plan your product launch after the approvals for your markets.

### What if my needs change?

Your firmware is open and your payload code is yours, so your application stays portable. Keep it separated from the driver behind the small `ts_platform_t` and event interfaces.

## Risks and how we help manage them

| Topic                      | Guidance                                                                                                                                                                                                     |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Hardware availability      | Use the simulated SDK demo for software evaluation; BEAR evaluation-kit dates will be published once build and supply readiness are confirmed. Target sales availability is April 1, 2027, a tentative date that is not confirmed; see the [R&D and release checklist](../BEAR_Module/RD_CHECKLIST.md) |
| API changes before 1.0     | Pin a version and read the [changelog](../CHANGELOG.md) before upgrading                                                                                                                                     |
| Timing on your target      | Budget bench time for your own electrical and timing measurements, as with any new peripheral                                                                                                                |
| Duplicate or lost messages | Put a sequence number in each payload. The driver never retries on its own                                                                                                                                   |

## A low-cost way to evaluate

1. **Day 1.** Run the [simulated demo](../Telemetry_Station_SDK/docs/GETTING_STARTED.md#2-see-it-work-with-no-hardware). Read [Design notes](../Telemetry_Station_SDK/docs/DESIGN.md).
2. **Week 1.** Design your real payload in the Payload Optimizer and size your plan in the calculator. Check the monthly cost against your budget.
3. **Prepare your firmware.** Port the SDK to your microcontroller and test application behavior against the simulated modem.
4. **When BEAR engineering samples are available.** Verify electrical integration and send test messages outdoors with a clear sky view.

## Questions to ask us

Contact [telemetrystation.com/contact](https://telemetrystation.com/contact). We welcome questions such as:

- What is the expected availability date for evaluation units?
- What are the service terms, pricing tiers and data export options?
- What is the long-term availability commitment for the module?
- Which certifications will cover my target markets, and when?
- Can we review your test plan, and take part in field trials?
