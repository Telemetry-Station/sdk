# 10 Requirements and Verification

Document ID: BEAR-RV-9704 | Revision: P2 | Status: Preliminary

Each requirement has an ID, a verification method (**T**est, **I**nspection, **A**nalysis, **D**emonstration) and a status. **Designed (Rev A)** means the current design implements the requirement, but hardware testing is pending. Native design files and internal traceability reports are not included in this SDK. Remaining open limits are marked **[Pending decision]**. Verified limits will be published in the [datasheet](../datasheet/BEAR-9704_Datasheet.md).

## Functional

| ID | Requirement | Method | Status |
|---|---|---|---|
| BEAR-REQ-001 | The module shall provide Iridium 9704 connectivity for sending and receiving messages | D | Defined |
| BEAR-REQ-002 | The module shall expose a serial interface at 230400 baud, 8N1 | T | Defined |
| BEAR-REQ-003 | The module shall report readiness on BOOTED | T | Defined |
| BEAR-REQ-004 | The IMEI shall be readable over the serial interface | D | Defined |
| BEAR-REQ-005 | The IMEI shall be printed on the module label | I | Target |
| BEAR-REQ-006 | The module shall not require a specific host MCU or vendor-specific host bus | I/T | Defined |

## Interface and power

| ID | Requirement | Method | Status |
|---|---|---|---|
| BEAR-REQ-010 | The module shall start only when ENABLE is asserted | T | Defined |
| BEAR-REQ-011 | The module shall complete the boot and shutdown handshake using ENABLE and BOOTED | T | Defined |
| BEAR-REQ-012 | Host inputs shall tolerate a host that is unpowered or high-impedance | T | Designed (Rev A): SN74LVC1G125 I_off buffers and 100 kΩ default-state resistors; verify at EVT |
| BEAR-REQ-013 | The module shall accept the published regulated DC input range | T | Designed (Rev A): 5.0 V ±5 %, ≥ 0.5 A source; verify at EVT/DVT |
| BEAR-REQ-014 | The module shall support transmit bursts without host supply collapse | T/A | Designed (Rev A): 2.5 F bank; VIN draw ≤ 0.44 A design value; burst margin requires EVT measurement |
| BEAR-REQ-015 | Disabled-state quiescent current shall be characterized and specified | T | Rev A test limit < 5 mA; final limit **[Pending decision]** after EVT measurement |

## RF

| ID | Requirement | Method | Status |
|---|---|---|---|
| BEAR-REQ-020 | The RF path shall support the Iridium L-band | T | Designed (Rev A): 50 Ω microstrip to SMA; test at EVT |
| BEAR-REQ-021 | The antenna configuration shall meet Iridium requirements | T | Maxtena M1621HCT-P-SMA reference selection; approved limits and regulatory review **[Pending decision]**; test planned |
| BEAR-REQ-022 | RF performance shall be documented for each supported antenna configuration | T | Configuration: SMA plus Maxtena reference antenna. Test limits **[Pending decision]** |

## Mechanical and environmental

| ID | Requirement | Method | Status |
|---|---|---|---|
| BEAR-REQ-030 | Operating temperature range shall suit outdoor remote sensing | T | Proposed target −40 °C to +85 °C **[Pending decision]**; confirm component limits and verify at DVT |
| BEAR-REQ-031 | The module shall pass vibration and shock tests (IEC 60068) | T | Planned |
| BEAR-REQ-032 | Mounting holes and connector positions shall match the controlled mechanical drawing | I | Designed (Rev A): 4 × M3 on 72 × 52 mm; approved customer model and dimensioned drawing release **[Pending decision]** |

## Firmware

| ID | Requirement | Method | Status |
|---|---|---|---|
| BEAR-REQ-040 | The SDK shall operate the module with no blocking calls | T | Defined (simulated modem) |
| BEAR-REQ-041 | The SDK shall wait for BOOTED low before restart or power removal | T | Defined (simulated modem) |
| BEAR-REQ-042 | The SDK shall not resend a message automatically | T | Defined (simulated modem) |
| BEAR-REQ-043 | The SDK shall provide a reference host port, initially the TI MSP430F169 | T | In development |
| BEAR-REQ-044 | The portable SDK core shall support additional host ports through the documented platform interface without requiring changes to the core | T | Defined (simulated modem) |

The MSP430F169 is the first reference host, not a product-use requirement. STM32, Nordic, Microchip, ESP32 and other MCU families are intended customer targets; each SDK port and its electrical compatibility must be validated independently. See the [firmware interface](07_Firmware_Interface.md).

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
