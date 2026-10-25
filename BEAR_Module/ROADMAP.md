# Roadmap

Document ID: BEAR-RM-001 | Revision: P1 | October 2026

BEAR Module products move through standard hardware development gates. Each gate has defined exit criteria, so you can see exactly what a given status means. Dates are published with each gate as it is scheduled.

| Gate | Name | What it means | Exit criteria | Status |
|---|---|---|---|---|
| G0 | Concept | Product definition, target users, interface and architecture | Product brief and system architecture approved | Complete |
| G1 | Design | Detailed electrical, mechanical and firmware design | Schematic and layout reviewed; datasheet Rev P2 with target values | In progress |
| G2 | EVT (engineering validation) | First hardware built and brought up on the bench | Power sequencing, UART link and IMEI readout demonstrated with the SDK | Coming soon |
| G3 | DVT (design validation) | Full characterization and pre-compliance testing | All requirements in the [verification plan](docs/10_Requirements_and_Verification.md) verified or dispositioned | Coming soon |
| G4 | Certification | Regulatory and Iridium approvals | Certificates issued for the target regions | Coming soon |
| G5 | PVT (production validation) | Pilot build on the production line | Yield and test coverage targets met | Coming soon |
| G6 | General availability | Orders accepted and shipped | Datasheet Rev A released; support and RMA live | Coming soon |

## Firmware and tools

| Item | Status |
|---|---|
| Telemetry Station SDK: driver, MSP430F169 port, host test suite | Alpha |
| SDK hardware validation on BEAR Module EVT units | Coming soon, at G2 |
| Iridium 9704 LaunchPad development kit sketch | Available. Validation on the LaunchPad is next |
| Payload Optimizer export for the SDK | See telemetrystation.com/payload-optimizer |
| Additional microcontroller ports | Planned; community contributions welcome |

## The BEAR Module family

The BEAR Module 9704 is the first product. Further family members will share the same host interface conventions, SDK and Telemetry Station workflow. They will be announced as they reach design gate G1.
