# Changelog

Use this page when deciding whether to upgrade. “Unreleased” describes changes on the development branch, not a published release or proof of hardware qualification.

**Upgrading?** Review API changes and rebuild all objects together. **First use?** [Start with the sample](Telemetry_Station_SDK/docs/GETTING_STARTED.md).

Telemetry Station SDK follows [semantic versioning](https://semver.org). Before 1.0, the API may change between minor versions.

## Unreleased

### Serial boundary and configuration hardening
- Reject impossible receive-callback byte counts through the existing `TS_ERR_IO` fault/shutdown path, matching the transmit-side contract checks.
- Reject embedded NUL bytes in serial lines with `TS_ERR_PROTOCOL`; discard the complete damaged line and resume at the next carriage-return boundary. Pending commands keep their original timeouts.
- Reject poll budgets that cannot fit a 16-bit loop counter and non-boolean automatic-IMEI flags at compile time; defaults and the public API are unchanged.
- Added fault regressions and compiler-driven configuration boundary checks. Clarified monotonic-clock and callback requirements without changing the first-use workflow.


### Guided sensor-to-insight setup
- Reviewed every Markdown page for purpose, prerequisites, clear next actions and evidence-based status; consolidated the first-use flow and corrected conflicting power/fault descriptions.
- Added `tools/try_sample.py` to prepare and run a simulated sensor reading. The demo uses the F169 sample codec, shows a decoded 21.5 °C preview and clearly marks simulated delivery and fictional identifiers.
- Added `tools/decode_sample.py` for the included schema, with transport-CRC verification by default and explicit payload-only decoding. This is a local decoder, not a provisioned cloud destination.
- Changed the F169 bench order and default application to one checkout per startup. `report_interval_ms` / `REPORT_INTERVAL_MS` equal to zero disables repeats; a positive interval explicitly enables them. Other existing orders retain their configured intervals.
- Added emulated single-checkout tests with and without an activated topic and console, and decoder failure cases. Hardware validation and a new TI target build remain required before release.
- Clarified the scope of customer hardware documentation.


### First-time setup guidance
- Added a plain-language entry path, current board-support limits, prerequisite checklist, sample-first activation/configuration steps and clear success/failure outcomes.
- Distinguished the computer simulation, engineering checkout, satellite delivery and application receipt; documented synthetic sample data and recurring-send behavior.
- Show charging as “charging power reserve; keep power connected” instead of a misleading BOOTED wait with a zero-second timer.
- Added measurable beginner-kit acceptance criteria and defined the planned customer documentation.

### Rev A integration consistency (2026-11-15)
- Added nonblocking `TS_CHARGING`, an optional `supply_ready` platform gate, a 120 s charge timeout and an independent shutdown timeout. A NULL gate explicitly requires external power qualification before every start. Template gates fail closed until implemented.
- Added `ts_bear_can_send()` to distinguish protocol readiness from accepting a send while an IMEI/signal query is pending. Examples check submission results and retain unaccepted payloads.
- F169 bench firmware waits a provisional 60 s from initialization with VIN already valid and continuously applied; qualification at EVT remains required. Report intervals now begin at completion so a long cold start cannot trigger back-to-back cycles.
- Corrected charge tolerance/calculation, PG versus STAT, powered-off host limitations, J1 wiring, polling budgets and fault/shutdown semantics across hardware and SDK documentation.
- API/ABI change for the next minor release: platform struct extended and state enum values changed. Rebuild all objects together, use symbolic state names and zero-initialize platform structs.
- Added charge timeout, cancellation, wraparound, late boot and send-readiness regression cases. Hardware validation is still pending.

### Flash-and-go BEAR checkout and stress tests
- The MSP430F169 bench firmware now runs a BEAR checkout every cycle and prints a timestamped PASS/WARN/CHECK/SKIP row for each step (module off, boot, serial link, SIM, radio, IMEI, signal, send, delivery, power down) on USART0 at 115200 8N1. The bench order and CCS project enable the console.
- Added an interrupt-driven F169 console ring; writes never block the modem and checkout text is never dropped.
- Fixed: MO messages larger than one segment could time out after 5 s per segment because each segment write reset the deadline.
- Fixed: after entering `TS_FAULT_HOLD` mid-poll, the driver kept processing already-buffered modem lines (could raise further errors or start an MT transfer). It now stops reading, as it does when powering down.
- Fixed in the checkout: IMEI and signal requests now wait for a driver that the application is using; an early clean power-off is no longer reported as BOOTED still high; the checkout no longer hangs when other code powers the module down.
- Added `tests/test_checkout.c` (full outcome permutations), `tests/test_f169_port.c` (real firmware on an emulated F169, including missing module, swapped wires, dead crystal and headless builds) and `tests/test_branches.c`.
- Coverage gate now requires 100% line and branch coverage (with reasoned waivers) across `src/`, the checkout, the F169 port and firmware, in two IMEI configurations. The feature matrix adds the `TS_QUERY_IMEI_ON_READY` CMake option.
- TI builds now stop when the static worst-case stack leaves less than 64 bytes of the 512-byte reserve (bench: 384 bytes).

### Embedded engineering update (2026-11-03)
- Added validated feature orders, isolated TI F169 build manifests and host/target feature matrix.
- Replaced the example placeholder with an actual Payload Optimizer Certus codec and golden vector.
- Added optional bounded LEA-M8S NAV-PVT acquisition; host wiring and configuration remain integration responsibilities.
- Fixed BOOTED loss/restart handling with conservative FAULT_HOLD; changed ts_f169_init() to return bool after bounded crystal startup.
- Disabled console by default; enabled console is nonblocking best effort. Removed activation printf use and made bench MT discard explicit.
- Added complete core line-coverage reporting, CCS read-only snapshot helper/tests, production evidence-record checks and hardware procedures.
- Updated the BEAR G1 roadmap and optional GNSS design input without claiming hardware qualification.

### Changed
- Separated the third-party Iridium 9704 LaunchPad messaging example from BEAR Module documentation, qualification, and roadmap; removed BEAR branding from the example's serial help.
- Aligned the evaluation guide and compliance plan with the R&D and release checklist: approval scope per market is pending, modules are sold as-is without a hardware warranty, and the tentative April 1, 2027 sales target is stated.

### Added
- Fault-injection tests (`tests/test_faults.c`) and a coverage gate (`tests/coverage.sh`, `TS_COVERAGE` option): 100% line coverage of `src/` with and without MT.
- Host build of the MSP430F169 port and example against a stub `msp430.h`, and editor include paths in `.vscode/c_cpp_properties.json`.

### Fixed
- Sanitizer flags now apply to the driver itself, and tests assert in Release builds.
- Removed unreachable code in the driver.

## 0.2.0

### Changed (API)
- Serial functions in `ts_platform_t` return `int16_t`.
- Information requests use the `ts_query_t` enum.

### Added
- `ts_bear_state()`, name functions for logging (`ts_state_name`, `ts_event_name`, `ts_result_name`) and `ts_version.h`.
- A bounded command builder. The driver core uses no printf-family functions and no heap, and a test enforces it.
- A shared modem simulator, a no-hardware demo (`simulated_demo`) and a port template.
- Optional watchdog support in the MSP430F169 port, and a duty-cycled example application.
- Documentation: getting started, design notes, evaluation guide, security policy and contributing guide.
- `SPDX-License-Identifier` headers in source files, and a third-party software and trademarks notice in the root README.
- CI with strict warnings, sanitizers and static analysis.

## 0.1.0

- First release of the driver, the MSP430F169 reference port, an Arduino messaging example for third-party Iridium 9704 LaunchPad hardware, and the BEAR Module 9704 documentation set.
