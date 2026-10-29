# BEAR Module

**Host-independent satellite connectivity for embedded systems.**

The BEAR Module family from Pacific Gyre, built under the Telemetry Station organization, is a host-independent satellite communications module designed for production use. You supply a microcontroller and a power source. The module supplies the modem, the power management around it, and (depending on configuration) a qualified Iridium antenna. The [Telemetry Station SDK](../Telemetry_Station_SDK) is the matching firmware.

The first member of the family is the **BEAR Module 9704**, built around the Iridium 9704 modem. The MSP430F169 is its first SDK reference host, not a requirement for using the module. The host interface is intended to support customer products using STM32, MSP430, Nordic, Microchip, ESP32 and other microcontrollers, subject to each host's electrical compatibility and SDK port.

Development is currently validating the design with existing embedded hardware while the BEAR hardware moves through its design gates. Production qualification is a release objective, not a claim about the current preliminary design.

This folder is the public engineering blueprint for the family: what the product is, how it is built, how it connects to your design, and how it is verified and supported. Proposed values are marked **[Pending decision]** and are not specifications; they require design review and verification before release. Contact [telemetrystation.com/contact](https://telemetrystation.com/contact) for questions.

## Status

| Item                                                      | Status                                                                                                          |
| --------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------- |
| Product definition and system architecture                | Defined                                                                                                         |
| Host interface and power sequencing                       | Defined                                                                                                         |
| Existing-hardware validation                              | In progress                                                                                                     |
| Firmware (Telemetry Station SDK)                          | Alpha (version 0.2)                                                                                             |
| Electrical specification and datasheet values             | Proposed targets under review; released with Rev A                                                              |
| Schematics, layout, mechanical drawings                   | Release package planned before general availability                                                             |
| Evaluation units and firm availability dates              | Planned; schedule pending engineering and supply readiness                                                      |
| Customer launch resources                                 | Platform quickstarts, decoder samples, public status/SLA and evaluation-kit schedule; see [roadmap](ROADMAP.md) |
| Production qualification and regulatory/Iridium approvals | Planned, see [compliance](docs/09_Compliance_and_Quality.md)                                                    |

Every document carries a status. **Defined** means the design decision is made. **In development** means work is underway. **[Pending decision]** marks an unresolved proposal or value requiring review; it is not a guaranteed specification. **Target** marks design intent, which is not yet a guaranteed specification.

## Start here

| If you are a...                        | Read                                                                                                                                                                                             |
| -------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Decision maker, buyer, product manager | [Product brief](docs/01_Product_Brief.md), [Ordering and support](docs/11_Ordering_and_Support.md), [Roadmap](ROADMAP.md)                                                                        |
| Electrical engineer                    | [Datasheet](datasheet/BEAR-9704_Datasheet.md), [Hardware interface](docs/03_Hardware_Interface.md), [Power management](docs/04_Power_Management.md), [Antenna and RF](docs/05_Antenna_and_RF.md) |
| Embedded firmware engineer             | [Firmware interface](docs/07_Firmware_Interface.md), [Integration guide](docs/06_Integration_Guide.md), [SDK](../Telemetry_Station_SDK)                                                          |
| Quality, regulatory, procurement       | [Compliance and quality](docs/09_Compliance_and_Quality.md), [Requirements and verification](docs/10_Requirements_and_Verification.md)                                                           |
| Mechanical engineer                    | [Mechanical and environmental](docs/08_Mechanical_and_Environmental.md)                                                                                                                          |

## Contents

```text
BEAR_Module/
├── README.md                      This page
├── ROADMAP.md                     Milestones from prototype to production
├── GLOSSARY.md                    Terms and abbreviations
├── datasheet/
│   └── BEAR-9704_Datasheet.md     Preliminary datasheet
├── docs/
│   ├── 01_Product_Brief.md
│   ├── 02_System_Architecture.md
│   ├── 03_Hardware_Interface.md
│   ├── 04_Power_Management.md
│   ├── 05_Antenna_and_RF.md
│   ├── 06_Integration_Guide.md
│   ├── 07_Firmware_Interface.md
│   ├── 08_Mechanical_and_Environmental.md
│   ├── 09_Compliance_and_Quality.md
│   ├── 10_Requirements_and_Verification.md
│   └── 11_Ordering_and_Support.md
├── hardware/                      Design-file index and templates
│   ├── interfaces/                Machine-readable signal list
│   ├── bom/                       Bill of materials template
│   ├── schematics/                Schematic package (block level now)
│   ├── pcb/                       Layout rules and stack-up
│   ├── mechanical/                Outline and 3D model
│   └── manufacturing/             Build, test and traceability
└── reference/                     External documents and standards
```

## How it works with the SDK

```text
 Your sensors ──> Your MCU running the Telemetry Station SDK ──> BEAR Module 9704 ──> Iridium
                      │ ENABLE / BOOTED / UART                           │
                      └──────────── reads IMEI ──> telemetrystation.com ─┘
                                                  (calculator, activation, data)
```

The SDK's platform interface maps directly to the module's signals: `power_start`/`power_stop` drive **ENABLE**, `boot_ready` reads **BOOTED**, and the serial functions use the module's UART. Porting to a new microcontroller means implementing those few functions. The MSP430F169 is the first reference host; it is not required in a customer product. The details are in the [firmware interface](docs/07_Firmware_Interface.md).

## Document control

All documents in this folder follow the same conventions:

- **Document ID and revision** at the top of each controlled document (for example `BEAR-DS-9704`, Rev P1).
- **Revision letters** (A, B, C) apply to released hardware. **P-numbers** (P1, P2) mark preliminary documents.
- Changes that affect form, fit or function are announced before they ship. See [Ordering and support](docs/11_Ordering_and_Support.md).
- The text in this folder is covered by the repository [MIT license](../LICENSE). Hardware design files will be released with their applicable license and revision metadata.

Product and company names are trademarks of their respective owners. Third-party names identify their respective products and do not imply affiliation or endorsement.
