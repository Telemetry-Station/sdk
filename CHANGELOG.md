# Changelog

Telemetry Station SDK follows [semantic versioning](https://semver.org). Before 1.0, the API may change between minor versions.

## Unreleased

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
