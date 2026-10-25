# 10 Requirements and Verification

Document ID: BEAR-RV-9704 | Revision: P1 | Status: Preliminary

Each requirement has an ID, a verification method (**T**est, **I**nspection, **A**nalysis, **D**emonstration) and a status. Numeric limits are published at Rev A and referenced from the [datasheet](../datasheet/BEAR-9704_Datasheet.md).

## Functional

| ID | Requirement | Method | Status |
|---|---|---|---|
| BEAR-REQ-001 | The module shall provide Iridium 9704 connectivity for sending and receiving messages | D | Defined |
| BEAR-REQ-002 | The module shall expose a serial interface at 230400 baud, 8N1 | T | Defined |
| BEAR-REQ-003 | The module shall report readiness on BOOTED | T | Defined |
| BEAR-REQ-004 | The IMEI shall be readable over the serial interface | D | Defined |
| BEAR-REQ-005 | The IMEI shall be printed on the module label | I | Target |

## Interface and power

| ID | Requirement | Method | Status |
|---|---|---|---|
| BEAR-REQ-010 | The module shall start only when ENABLE is asserted | T | Defined |
| BEAR-REQ-011 | The module shall complete the boot and shutdown handshake using ENABLE and BOOTED | T | Defined |
| BEAR-REQ-012 | Host inputs shall tolerate a host that is unpowered or high-impedance | T | In development |
| BEAR-REQ-013 | The supply range shall allow common battery and DC sources | T | Coming soon |
| BEAR-REQ-014 | The module shall support transmit bursts without host supply collapse | T/A | In development |
| BEAR-REQ-015 | Quiescent current while disabled shall be minimal | T | Coming soon |

## RF

| ID | Requirement | Method | Status |
|---|---|---|---|
| BEAR-REQ-020 | The RF path shall support the Iridium L-band | T | Defined |
| BEAR-REQ-021 | The antenna configuration shall meet Iridium requirements | T | Planned |
| BEAR-REQ-022 | RF performance shall be documented with the supported antennas | T | Coming soon |

## Mechanical and environmental

| ID | Requirement | Method | Status |
|---|---|---|---|
| BEAR-REQ-030 | Operating temperature range shall suit outdoor remote sensing | T | Coming soon |
| BEAR-REQ-031 | The module shall pass vibration and shock tests (IEC 60068) | T | Planned |
| BEAR-REQ-032 | Mounting holes and connector positions shall match the published drawing | I | Coming soon |

## Firmware

| ID | Requirement | Method | Status |
|---|---|---|---|
| BEAR-REQ-040 | The SDK shall operate the module with no blocking calls | T | Defined (simulated modem) |
| BEAR-REQ-041 | The SDK shall wait for BOOTED low before restart or power removal | T | Defined (simulated modem) |
| BEAR-REQ-042 | The SDK shall not resend a message automatically | T | Defined (simulated modem) |
| BEAR-REQ-043 | The SDK shall run on the MSP430F169 | T | In development |

## Compliance and quality

| ID | Requirement | Method | Status |
|---|---|---|---|
| BEAR-REQ-050 | The module shall meet the approvals in [09](09_Compliance_and_Quality.md) | T | Planned |
| BEAR-REQ-051 | Every unit shall pass an automated functional test | T | Planned |
| BEAR-REQ-052 | Every unit shall be traceable by serial number | I | Planned |

## Verification phases

| Phase | Activity |
|---|---|
| EVT | Bench bring-up, interface and power verification, SDK on target |
| DVT | Environmental, EMC pre-scan, RF performance, full requirements pass |
| Certification | Accredited laboratory testing |
| PVT and production | Process validation, functional test at every unit |
