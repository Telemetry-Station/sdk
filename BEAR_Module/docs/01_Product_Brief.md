# 01 Product brief: what BEAR does for your device

Document ID: BEAR-PB-9704 | Revision: P4 (2026-11-15) | Status: Preliminary

BEAR Module 9704 is being built to send readings from sensors beyond cellular and Wi-Fi coverage. It combines the satellite modem and its power circuitry; the SDK handles communication from your microcontroller.

**[See the sample →](../../Telemetry_Station_SDK/docs/GETTING_STARTED.md#2-see-it-work-with-no-hardware)** You can evaluate the software without hardware or service activation.

## What the system includes

| Part | Its job | Current status |
|---|---|---|
| BEAR Module 9704 | Connects your device to Iridium through an external antenna | Rev A design review; hardware tests pending |
| Power circuitry | Stores energy for brief transmit-current bursts | 5 V / ≥0.5 A source is the design requirement; burst margin and charge time require measurement |
| Telemetry Station SDK | Starts the modem, transfers bytes, reports delivery and shuts down | Alpha C99 software; MSP430F169 reference integration |
| Included sample and decoder | Demonstrates a sequence number and temperature reading | Computer demo and local decoding available |
| Telemetry Station service | Activation and the intended data-delivery path | Confirm destination, service terms and availability for your evaluation |

Your product supplies the sensor, microcontroller, compatible supply and connections. Firmware runs on your microcontroller, not inside BEAR. The host interface uses serial data and two control signals; it is designed to support different MCU families, but each requires a tested software adapter and electrical compatibility review.

## Start with a result

1. **Try one sample on your computer.** See the example 21.5 °C reading.
2. **Check your hardware path.** The [setup guide](../../Telemetry_Station_SDK/docs/GETTING_STARTED.md#3-pick-your-path) identifies the reference host and prerequisites.
3. **Read and activate your actual module ID.** Keep the included sample format initially.
4. **Send once.** Check satellite delivery and the decoded value at your configured destination.
5. **Connect your sensor.** Reuse the format if it fits; otherwise generate a new codec and configure its matching decoder.

No live hardware setup has yet established this full journey. The [release checklist](../RD_CHECKLIST.md) tracks what remains before it can be sold as a beginner-ready kit.

## Is it a fit?

The intended applications send occasional small readings: environmental monitoring, remote infrastructure, agricultural sensors, marine/polar instruments and asset tracking. Antenna placement, available power and reporting frequency determine suitability. Prototype and field measurements are required before making performance commitments.

The modem's large-message capability does not describe the SDK's default capacity: the reference configuration reserves 64 application bytes per message. [Memory and configuration](../../Telemetry_Station_SDK/README.md#configuration) must be reviewed for the selected host.

## Availability

Evaluation kits, pilot supply and production configurations are planned. Contents, price and lead times are not confirmed. Target sales availability is April 1, 2027, subject to validation, approvals and production readiness. Public interfaces are documented here.

**Next:** [request evaluation details](11_Ordering_and_Support.md) with your sensor, host, power source, location and reporting interval.
