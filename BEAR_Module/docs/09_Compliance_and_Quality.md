# 09 Compliance and Quality

Document ID: BEAR-CQ-9704 | Revision: P1 | Status: Preliminary

Customers integrating a radio module need to know which approvals exist, what is inherited from the modem, and what is left to them. This document lays out the plan. Certificates are published here as they are issued.

## Certification plan

| Area | Scope | Standards (target) | Status |
|---|---|---|---|
| Iridium approval | Network access for the module and its antenna configurations | Iridium requirements for integrated devices | Planned |
| United States | Radio and EMC | FCC 47 CFR Part 15, applicable Part 25 requirements | Planned |
| Canada | Radio and EMC | ISED RSS standards, ICES-003 | Planned |
| European Union | Radio and EMC, safety | RED 2014/53/EU with EN 301 441, EN 301 489, EN 62368-1 | Planned |
| United Kingdom | Radio and EMC | UKCA | Planned |
| Australia and New Zealand | EMC | RCM | Planned |
| Other regions | On demand | To be announced | Coming soon |

The exact standards list and test scope is confirmed with the accredited test laboratory at the certification gate (G4). The modem's own approvals reduce the module-level scope. Details are published with the certificates.

## Environmental and material compliance

| Requirement | Status |
|---|---|
| RoHS (EU 2011/65/EU) | Design requirement |
| REACH substances reporting | Design requirement |
| WEEE labeling | Planned |
| Conflict minerals reporting | Coming soon |
| Country of origin | Published with each order |

## Electromagnetic compatibility and safety (design)

| Test | Standard | Status |
|---|---|---|
| ESD immunity | IEC 61000-4-2 | Planned (DVT) |
| Radiated immunity | IEC 61000-4-3 | Planned (DVT) |
| Electrical fast transient | IEC 61000-4-4 | Planned (DVT) |
| Surge | IEC 61000-4-5 | Planned (DVT) |
| Conducted emissions and radiated emissions | CISPR 32 / FCC Part 15 | Planned (DVT) |

## Quality system

| Topic | Approach |
|---|---|
| Design process | Gated development (see [Roadmap](../ROADMAP.md)), design reviews at each gate, requirements traceability ([10](10_Requirements_and_Verification.md)) |
| Manufacturing quality | Contract manufacturing with IPC-A-610 Class 2 workmanship and IPC J-STD-001 soldering, targeting an ISO 9001 certified partner |
| PCB | Built to IPC-6012 |
| Testing | Every unit passes an automated functional test, including a power-sequencing check and a UART loopback to the modem |
| Traceability | Serial number, lot and date code, linked to the modem IMEI and test results |
| Configuration control | Hardware revision letters. Changes recorded in the revision history. See [11](11_Ordering_and_Support.md) |
| Non-conformance | Defined process for returns, failure analysis and corrective action |

## Security and data integrity

| Topic | Position |
|---|---|
| Transport | Iridium network link to the Iridium gateway |
| Integrity check | The SDK appends a CRC16 to every message to detect corruption. It is not authentication |
| Authentication and encryption | Application-level options (for example, authenticated payloads) are documented with the SDK as they become available |
| Firmware updates | The module's host interface does not accept firmware. Host firmware is yours |
| Vulnerability reports | Use the repository's [security policy](../../SECURITY.md) |

## Export and legal

| Item | Status |
|---|---|
| Export classification (ECCN, HS code) | Coming soon, with ordering information |
| Warranty | Coming soon |
| Hardware design license | Published with the design files |
