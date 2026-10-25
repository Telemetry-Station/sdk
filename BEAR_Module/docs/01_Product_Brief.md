# 01 Product Brief

Document ID: BEAR-PB-9704 | Revision: P1 | Status: Preliminary

## The problem

Adding satellite messaging to a remote sensor is harder than the sensor itself. Teams must source a modem, design a power supply that survives transmit bursts, sequence the modem on and off safely, find and qualify an antenna, write a serial protocol driver, size a data plan, and wire up cloud delivery. Each step is a place for a project to slip.

## The BEAR Module answer

A BEAR Module collapses that work into one board and one firmware library:

| You need | BEAR Module provides |
|---|---|
| A satellite modem | Iridium 9704 modem (BEAR Module 9704) |
| A power supply that handles transmit bursts | On-board power management |
| An antenna | Antenna connection, or a qualified Iridium antenna, depending on configuration |
| A driver | The open-source [Telemetry Station SDK](../../Telemetry_Station_SDK) |
| A small, cheap message format | [Payload Optimizer](https://telemetrystation.com/payload-optimizer) |
| A plan and activation | [Calculator](https://telemetrystation.com/calculators/certus) and [activation](https://telemetrystation.com/activate/certus) at telemetrystation.com |
| Your data | Telemetry Station data infrastructure |

## Who it is for

- Environmental, ocean, polar, agricultural and infrastructure monitoring.
- Asset and fleet tracking where cellular coverage is absent or unreliable.
- OEMs that want satellite connectivity as a feature of their own product.
- Research teams that need a documented, reproducible connectivity layer.

The same module serves a single prototype and a production run. The host interface, firmware and activation flow are identical at both scales.

## Key capabilities

| Capability | Description | Status |
|---|---|---|
| Iridium 9704 modem | Messages up to 100,000 bytes over the Iridium network | Defined |
| Power management | Input conditioning, regulation, energy storage for transmit bursts, protected on/off sequencing | Defined |
| Simple host interface | Power, an enable line, a booted line and one UART | Defined |
| Any microcontroller | 3-wire style serial plus two control lines; no vendor-specific bus | Defined |
| Antenna options | Connector for your antenna, or a qualified antenna | In development |
| IMEI on the label | Each module carries its IMEI so activation can start before firmware exists | In development |
| Open firmware | MIT-licensed SDK with a portable C99 driver and a hardware abstraction layer | Alpha |
| Telemetry Station workflow | Payload design, sizing, activation and data in one flow | Available at telemetrystation.com |

## The end-to-end workflow

1. **Design the message** in the Payload Optimizer. Download `payload.c` and `payload.h`. Note the bytes per message.
2. **Integrate the module.** Wire power, ENABLE, BOOTED and the UART to your microcontroller ([integration guide](06_Integration_Guide.md)).
3. **Add the SDK.** Implement the platform interface for your MCU and connect your encoder.
4. **Read the IMEI.** The firmware prints it and the next steps when the modem is ready.
5. **Size and activate.** Enter your needs in the calculator, then activate with the IMEI. You receive your topic ID.
6. **Send.** Messages flow to Telemetry Station.

You can use the BEAR Module with other data services. The workflow above is the one we design, document and support end to end.

## Commercial scale

| Stage | What you get | Status |
|---|---|---|
| Evaluate | Evaluation unit with documentation and SDK | Coming soon |
| Pilot | Small quantities with full support and a stable configuration | Coming soon |
| Production | Volume pricing, controlled revisions, change notification, supply commitments | Coming soon |

Pricing, lead times and terms are published at general availability. See [Ordering and support](11_Ordering_and_Support.md).
