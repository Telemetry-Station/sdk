<p align="right"><img src="../assets/TelemetryStation.svg" width="112" alt="Telemetry Station" /></p>

# BEAR Module 9704 Datasheet

Use this reference to check power, pins, dimensions and timing for BEAR Module 9704. These are preliminary design values; this document does not establish that hardware has passed validation.

**New to the product?** Read the [product brief](../docs/01_Product_Brief.md). **Connecting hardware?** Follow the [integration checklist](../docs/06_Integration_Guide.md), then use these tables to verify compatibility.

Document ID: BEAR-DS-9704 | Revision: P4 (2026-11-15) | Status: Preliminary (Rev A design values; not yet measured)

Preliminary information. Specifications may change. Values below are the Rev A design values, not measured or released specifications. Items that can only be established by measurement are marked **Characterize at EVT**. Remaining open items, including vendor limits awaiting publication clearance, are marked **[Pending decision]**.

The BEAR Module 9704 is designed as a host-independent satellite communications module for production-qualified deployment. It is currently in development and is not yet production-qualified. The TI MSP430F169 is the first SDK reference host, not a requirement for using the module.

## Features

- Iridium 9704 satellite modem for IoT messaging
- Power circuitry with boot and shutdown sequenced by the host/SDK
- 2.5 F supercapacitor energy storage for transmit bursts, with a 5 V / ≥0.5 A source design requirement; burst margin remains unmeasured
- UART interface, 230400 baud 8N1
- ENABLE and BOOTED handshake signals
- SMA antenna connector (50 Ω); Maxtena M1621HCT-P-SMA reference antenna
- Buffered 3.3 V host I/O, 5.5 V-tolerant and Ioff-protected when the buffer supply is off; unpowered-host compatibility requires verification
- Telemetry Station SDK in portable C99, with an MSP430F169 first reference port
- Host-independent interface: UART, ENABLE and BOOTED; no specific host MCU required

## Applications

Remote environmental monitoring, asset tracking, marine and polar buoys, agriculture, utilities, infrastructure.

## Block diagram

```mermaid
flowchart LR
  VIN --> PWR[Power management]
  PWR --> MODEM[Iridium 9704]
  MODEM --> RF[RF path] --> ANT
  HOST[Host MCU] -- ENABLE --> PWR
  MODEM -- BOOTED --> HOST
  HOST <-- "UART TXD/RXD" --> MODEM
```

## Signals

| Signal | Direction (module) | Description |
|---|---|---|
| VIN | In | Supply |
| GND | Power | Ground |
| ENABLE | In | High starts the module. Low starts shutdown |
| BOOTED | Out | High when boot completes; not proof of network readiness. Low after shutdown |
| TXD | Out | Serial data to host |
| RXD | In | Serial data from host |
| TX_ACTIVE | Out | Optional transmit indicator |
| WAKE_IN, WAKE_OUT | Reserved | Future use; Rev A test pads TP10/TP9 only |
| ANT | RF | Antenna connection |

Host connector J1: Molex Micro-Fit 3.0 43045-1012 (2 × 5, 3.0 mm pitch, keyed). Mating housing 43025-1000 with 43030-0007 crimp terminals.

| J1 pin | Signal | J1 pin | Signal |
|---|---|---|---|
| 1 | VIN | 2 | VIN |
| 3 | GND | 4 | GND |
| 5 | ENABLE | 6 | BOOTED |
| 7 | TXD | 8 | RXD |
| 9 | TX_ACTIVE | 10 | Reserved (no connect) |

RF connector J2: Amphenol 132134 SMA jack, vertical. See [03 Hardware interface](../docs/03_Hardware_Interface.md).

## Ratings and characteristics

| Parameter | Min | Typ | Max | Basis |
|---|---|---|---|---|
| Absolute maximum supply voltage | −20 V (reverse) | | 6.0 V continuous | Q1 DMP3099L V_GS ±20 V; D1 SMAJ6.0A V_RWM 6.0 V (conducts from 6.67 V) |
| Recommended supply voltage (VIN) | 4.75 V | 5.0 V | 5.25 V | Rev A power design |
| Supply source capability | 0.5 A | | | Charger I_CHG 0.36 / 0.40 / 0.44 A before resistor tolerance and quiescent current; validate total input current at EVT |
| Host input V_IH (ENABLE, RXD) | 2.0 V | | 5.5 V | SN74LVC1G125 at V_CC 3.0–3.6 V |
| Host input V_IL (ENABLE, RXD) | 0 V | | 0.8 V | SN74LVC1G125 |
| Host output V_OH (BOOTED, TXD, TX_ACTIVE) | 2.4 V at −16 mA | 3.3 V | | SN74LVC1G125 at V_CC 3.0 V |
| Host output V_OL | | | 0.55 V at 24 mA | SN74LVC1G125 |
| Unpowered behavior | | | ±10 µA | I_off at buffer V_CC = 0 V only; powered module outputs may back-power an unpowered host |
| Current, disabled (ENABLE low, bank full) | | | | Characterize at EVT; Rev A test limit < 5 mA |
| Current, transmit | | | Characterize at EVT | Charger design range 0.36–0.44 A before resistor tolerance and overhead; the bank supplies bursts. Modem burst current is not published by Iridium; Rev A design envelope 2.0 A from the bank. Characterize at EVT |
| Operating temperature | −40 °C target | | +85 °C target | Confirm component limits and verify at DVT **[Pending decision]** |

## Timing

| Parameter | Description | Value |
|---|---|---|
| t_VIN | Supply applied to ENABLE allowed | Qualified stored energy required. About 26 s nominal; ≥38 s calculated maximum-capacitance/minimum-current case before thermal effects and load. F169 bench wait: 60 s provisional; worst-case limit requires EVT |
| t_BOOT | ENABLE high to BOOTED high | Not published by Iridium. Characterize at EVT; the SDK uses a timeout |
| t_DOWN | ENABLE low to BOOTED low | Not published by Iridium. Characterize at EVT |
| t_UART | BOOTED high to first serial command | Not published by Iridium. Characterize at EVT |

Wait for BOOTED high before using the UART and for TS_EVENT_STOPPED (ENABLE and BOOTED low) before removing VIN. See [04 Power management](../docs/04_Power_Management.md).

## RF, mechanical, environmental

| Parameter | Value |
|---|---|
| Frequency | Iridium L-band, 1616 to 1626.5 MHz |
| Antenna impedance | 50 Ω; 0.34 mm microstrip on fabricator-controlled impedance |
| Antenna | Maxtena M1621HCT-P-SMA (2.8 dBic) reference selection; approved gain limits and installation conditions **[Pending decision]** |
| Conducted transmit power (modem) | Publish permitted specifications and verify at EVT **[Pending decision]** |
| Dimensions | 80 × 60 × 1.6 mm PCB, R5 corners; 4 × M3 holes (Ø3.2 mm) on a 72 × 52 mm pattern |
| Mass | Weigh at EVT **[Pending decision]** |
| Environmental | See [08](../docs/08_Mechanical_and_Environmental.md) |

## Regulatory

See [09 Compliance and quality](../docs/09_Compliance_and_Quality.md).

## Ordering

See [11 Ordering and support](../docs/11_Ordering_and_Support.md).

## Revision history

| Revision | Date | Change |
|---|---|---|
| P4 | 2026-11-15 | Clarify integration steps, design-value status, BOOTED meaning and host-owned sequencing |
| P3 | 2026-11-15 | Correct supply qualification and unpowered-host limits; synchronize SDK lifecycle requirements |
| P2 | 2026-11-07 | Rev A design values: connector and pinout, supply, logic levels, protection, t_VIN, RF and mechanical. Unpublished modem currents and timing marked for EVT characterization |
| P1 | October 2026 | Preliminary release |

**Next:** [connect using the integration checklist](../docs/06_Integration_Guide.md); stop at the first unmet prerequisite.
