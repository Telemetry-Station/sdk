<p align="left"><img src="assets/TelemetryStation.svg" width="112" alt="Telemetry Station" /></p>

# BEAR Module 9704 R&D and Release Checklist

Document ID: BEAR-RD-9704 | Revision: P3 | Status: Preliminary

Updated: 2026-11-07 | Current stage: **G1 - Design** (Rev A design review required)

## Availability

**Target sales availability: April 1, 2027 (2027-04-01).**

This date is tentative, not a confirmed release date. BEAR Module 9704 is in development and is not yet production-qualified. Availability depends on hardware validation, applicable regulatory and Iridium approvals, and production readiness. Dates for the remaining milestones and BEAR evaluation kits are not yet confirmed.

## Development status

Checked items are documented or available as described. Unchecked items are remaining release deliverables. A defined interface or requirement does not mean that hardware testing is complete. Proposed specifications remain **[Pending decision]** until finalized and verified.

| Stage | Status | Customer milestone |
|---|---|---|
| G0 - Concept | Complete | Product scope and architecture defined |
| G1 - Design | In progress: Rev A KiCad design complete, design review required | Detailed hardware design and interface specifications |
| G2 - Engineering validation (EVT) | Planned | BEAR engineering units demonstrated with the SDK |
| G3 - Design validation (DVT) | Planned | Electrical, RF, environmental, and host compatibility verified |
| G4 - Certification | Planned | Applicable approvals for supported regions and configurations |
| G5 - Production validation (PVT) | Planned | Production build, unit testing, and traceability validated |
| G6 - General availability | Tentative target: 2027-04-01 | Orderable products, released documentation, and customer support |

See the [roadmap](ROADMAP.md) for milestone details and the [requirements and verification plan](docs/10_Requirements_and_Verification.md) for individual requirements.

## Product definition

- [x] Iridium 9704-based module and intended remote sensing/OEM applications described in the [product brief](docs/01_Product_Brief.md).
- [x] Host-independent architecture with an external customer MCU described in the [system architecture](docs/02_System_Architecture.md).
- [x] Signal functions, 230400 baud/8N1 UART, and ENABLE/BOOTED handshake defined in the [hardware interface](docs/03_Hardware_Interface.md) and [signal list](hardware/interfaces/bear-9704_signals.csv). Rev A electrical limits and connector pin assignments are specified there; verification is pending at EVT.
- [x] Input protection, regulation, burst-energy storage, and sequencing functions described in [power management](docs/04_Power_Management.md). Rev A component design is complete; measured performance is pending EVT.
- [x] [Preliminary datasheet](datasheet/BEAR-9704_Datasheet.md) and [requirements with verification methods](docs/10_Requirements_and_Verification.md) available.
- [x] Portable C99 alpha [SDK](../Telemetry_Station_SDK/README.md), simulated-modem tests, and first MSP430F169 reference port available. Validation on BEAR hardware is pending.
- [x] [Key-component BOM](hardware/bom/BEAR-9704_BOM.csv), [signal list](hardware/interfaces/bear-9704_signals.csv) and [manufacturing overview](hardware/manufacturing/README.md) available.
- [ ] Launch configurations, supported regions, and evaluation-kit contents finalized.

## Detailed design

- [x] Input-voltage range (5.0 V ±5 %), source requirement (≥ 0.5 A), burst-energy storage (2.5 F supercapacitor bank), and protection (P-FET reverse polarity, 6.0 V TVS, 18 V charger OVP) specified in [power management](docs/04_Power_Management.md).
- [x] Host connector (Molex Micro-Fit 3.0 43045-1012), mating parts, pin assignments, 3.3 V logic thresholds, absolute ratings, and unpowered-host behavior specified in the [hardware interface](docs/03_Hardware_Interface.md).
- [x] Antenna/RF configuration (SMA, 50 Ω; Maxtena M1621HCT-P-SMA reference antenna), connection requirements, and integration constraints specified in [antenna and RF](docs/05_Antenna_and_RF.md).
- [x] Dimensions (80 × 60 mm), mounting (4 × M3), and connector positions specified in [mechanical and environmental](docs/08_Mechanical_and_Environmental.md). Environmental limits require DVT validation; approved mechanical files will be published separately.
- [x] Rev A schematic, routed four-layer PCB, populated BOM and fabrication outputs completed for design review. Native files and internal review reports are not included in this SDK.
- [x] Rev P2 preliminary datasheet available with Rev A design values.
- [ ] Rev A design review closed, including confirmation of the modem land pattern.
- [ ] Modem supply current and t_BOOT / t_DOWN / t_UART timing measured; Iridium does not publish them.

## Engineering validation (EVT)

- [ ] BEAR engineering units built and brought up.
- [ ] Power rails, inrush, transmit-burst behavior, thermal performance, and power-loss/shutdown behavior tested.
- [ ] ENABLE/BOOTED sequencing, UART communication, IMEI readout, timing, disabled-state current, and unpowered-host behavior tested on BEAR units.
- [ ] SDK send/receive, delivery reporting, fault handling, restart, and safe shutdown demonstrated with the MSP430F169 reference host on BEAR hardware.
- [ ] Antenna/link operation and end-to-end activation, payload delivery, and decoding demonstrated.

## Design validation (DVT)

- [ ] Electrical ratings, operating-state current/energy, timing, and thermal derating characterized across supported conditions.
- [ ] RF performance verified for each supported antenna configuration.
- [ ] Temperature, humidity, shock/vibration, and reliability testing completed for the specified operating environment.
- [ ] Applicable EMC, ESD, immunity, and safety pre-compliance testing completed.
- [ ] Advertised host/SDK compatibility verified, with tested platform and toolchain versions identified.
- [ ] Requirements verification and representative field trials completed; release-critical defects resolved.

## Certification and compliance

- [ ] Applicable standards and approval scope confirmed for each supported market and configuration.
- [ ] Applicable regulatory and Iridium approvals completed, with certificate identifiers, approved antennas, and integration conditions available.
- [ ] Applicable material compliance, labeling, country-of-origin, and export/customs information available.
- [ ] Customer documentation identifies module approvals and any remaining end-product certification obligations. See [compliance and quality](docs/09_Compliance_and_Quality.md).

## Production readiness

- [ ] Production-intent pilot build completed with validated assembly quality, yield, and test coverage.
- [ ] Automated per-unit functional testing validated, including power sequencing, modem communication, and IMEI readout.
- [ ] Serial/lot traceability linked to IMEI, hardware revision, and unit test results.
- [ ] Production configuration consistent with the qualified design and applicable approvals.
- [ ] ESD-protective packaging, labeling, production supply, and fulfillment ready.
- [ ] Released KiCad schematic and PCB source, fabrication/assembly files, BOM, mechanical drawings/models, and applicable licenses available through the [hardware index](hardware/README.md).

## Customer release

- [ ] Rev A datasheet PDF available with verified ratings, timing, pinout, mechanical data, approved configurations, and revision history.
- [ ] Tested Arduino, STM32, and Raspberry Pi quickstarts, sample payload decoders, and matching test vectors available.
- [ ] SDK release baseline, compatibility, known limitations, and release notes available in the [changelog](../CHANGELOG.md).
- [ ] Orderable part numbers, kit contents, pricing, lead times, and confirmed evaluation-kit dates available in [ordering and support](docs/11_Ordering_and_Support.md).
- [ ] Sales terms (sold as-is, no hardware warranty), support, lifecycle, and product-change notification terms available before sales.
- [ ] Product/service status page and approved service-level terms available.
- [ ] Required validation, approvals, and production readiness completed; customer sales availability confirmed.

## Revision history

| Date | Revision | Summary |
|---|---|---|
| 2026-11-07 | P3 | Rev A KiCad design complete (design review required). Detailed-design inputs filled from Rev A; Altium replaced by KiCad. |
| 2026-11-03 | P2 | Progress update: design in progress, SDK bench checkout available; no production gate advanced. |
| 2026-10-29 | P1 | Initial product-readiness checklist. Current status: design in progress. Tentative sales target: April 1, 2027. |

## Progress update — 2026-11-07

G1 (design) remains **in progress** at **DESIGN REVIEW REQUIRED**. The Rev A KiCad design is complete for review.
Approved design deliverables will be published separately; native files and internal reports are not part of this SDK.
Boards are not ordered until the modem land pattern is confirmed.
No hardware has been built. Validation on BEAR hardware is **NOT YET RUN**. The April 1, 2027 availability target
remains tentative.
