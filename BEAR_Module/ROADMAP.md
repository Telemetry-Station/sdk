# Roadmap

Document ID: BEAR-RM-001 | Revision: P1 | October 2026

BEAR Module products move through standard hardware development gates toward a host-independent, production-qualified satellite communications module. Each gate has defined exit criteria, so you can see exactly what a given status means. Dates are published with each gate as it is scheduled. Qualification is not complete while the product is in G1.

| Gate | Name | What it means | Exit criteria | Status |
|---|---|---|---|---|
| G0 | Concept | Product definition, target users, interface and architecture | Product brief and system architecture approved | Complete |
| G1 | Design | Detailed electrical, mechanical and firmware design | Schematic and layout reviewed; datasheet Rev P2 with target values | In progress |
| G2 | EVT (engineering validation) | First hardware built and brought up on the bench | Power sequencing, UART link and IMEI readout demonstrated with the SDK | Planned; schedule **[Pending decision]** |
| G3 | DVT (design validation) | Full characterization and pre-compliance testing | All requirements in the [verification plan](docs/10_Requirements_and_Verification.md) verified or dispositioned | Planned; schedule **[Pending decision]** |
| G4 | Certification | Regulatory and Iridium approvals | Certificates issued for the target regions | Planned; scope and schedule **[Pending decision]** |
| G5 | PVT (production validation) | Pilot build on the production line | Yield and test coverage targets met | Planned; schedule **[Pending decision]** |
| G6 | General availability | Orders accepted and shipped | Rev A datasheet PDF, support/RMA, public status page and SLA, customer quickstarts and decoder examples, published evaluation-kit dates, changelog, and the controlled schematic and Altium PCB design package available | Planned; schedule **[Pending decision]** |

## Firmware and tools

| Item | Status |
|---|---|
| Telemetry Station SDK: driver, MSP430F169 reference port, host test suite | Alpha |
| SDK hardware validation on BEAR Module EVT units | Planned at G2; date **[Pending decision]** |
| Existing-hardware validation | In progress using available embedded hardware |
| Payload Optimizer export for the SDK | See telemetrystation.com/payload-optimizer |
| Host MCU compatibility | Designed to be host-independent; STM32, MSP430, Nordic, Microchip, ESP32 and other MCU ports are intended and validated individually |
| Customer quickstarts | Arduino, STM32 and Raspberry Pi guides planned; exact supported versions and validation status tracked per guide |
| Sample decoders | Example decode projects and test vectors planned for publication |
| Changelog | Public repository [changelog](../CHANGELOG.md) |
| Public service status and SLA | Public status page and service-level terms planned; URL, availability objectives and response targets **[Pending decision]** |
| Evaluation-kit availability | Firm public dates are a launch-readiness deliverable; date **[Pending decision]** until build and supply readiness are confirmed |

The MSP430F169 is the first reference host. The design is currently being validated with existing embedded hardware while the host-independent module design progresses.

## The BEAR Module family

The BEAR Module 9704 is the first product. Further family members will share the same host interface conventions, SDK and Telemetry Station workflow. They will be announced as they reach design gate G1.
