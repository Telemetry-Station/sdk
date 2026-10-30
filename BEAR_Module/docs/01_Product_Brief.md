# 01 Product Brief

Document ID: BEAR-PB-9704 | Revision: P1 | Status: Preliminary

## The problem

Adding satellite messaging to a remote sensor is harder than the sensor itself. Teams must source a modem, design a power supply that survives transmit bursts, sequence the modem on and off safely, find and qualify an antenna, write a serial protocol driver, size a data plan, and wire up cloud delivery. Each step is a place for a project to slip.

## The BEAR Module answer

The BEAR Module is a host-independent satellite communications module designed to become a production-qualified product. Its hardware interface does not require a particular host MCU or vendor-specific bus. The first product, BEAR Module 9704, combines an Iridium 9704 with on-module power management and a simple UART/control interface:

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

The same module is intended to serve a single prototype and a production run. The host interface and activation flow are designed to remain consistent at both scales. Production qualification is not complete; it is an explicit release objective in the [roadmap](../ROADMAP.md).

## Key capabilities

| Capability | Description | Status |
|---|---|---|
| Iridium 9704 modem | Messages up to 100,000 bytes over the Iridium network | Defined |
| Power management | Input conditioning, regulation, energy storage for transmit bursts, protected on/off sequencing | Defined |
| Simple host interface | Power, an enable line, a booted line and one UART | Defined |
| Host-independent interface | UART plus ENABLE and BOOTED; no vendor-specific bus or required host MCU | Defined |
| Customer MCU compatibility | Intended for STM32, MSP430, Nordic, Microchip, ESP32 and other MCUs with a compatible UART/GPIO interface and SDK port | Design target |
| Production qualification | Qualification and release evidence through EVT, DVT, certification and PVT | In development |
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

## Host and validation status

The TI MSP430F169 is the SDK's first reference host and a current validation target. SDK validation on BEAR engineering units is planned at G2 (EVT); see the [roadmap](../ROADMAP.md).

## Commercial scale

| Stage | What you get | Status |
|---|---|---|
| Evaluate | Evaluation unit, quickstarts and engineering documentation; firm kit dates published when supply and validation are confirmed | Planned; date **[Pending decision]** |
| Pilot | Small quantities with support and a stable configuration | Planned; terms **[Pending decision]** |
| Production | Volume pricing, controlled revisions, change notification, supply commitments and published service status/SLA | Planned; terms **[Pending decision]** |

Pricing, lead times and terms are published at general availability. See [Ordering and support](11_Ordering_and_Support.md).
