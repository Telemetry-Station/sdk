# Changelog

Telemetry Station SDK follows [semantic versioning](https://semver.org). Before 1.0, the API may change between minor versions.

## Unreleased

### Changed
- Separated the third-party Iridium 9704 LaunchPad messaging example from BEAR Module documentation, qualification, and roadmap; removed BEAR branding from the example's serial help.

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

- First release of the driver, the MSP430F169 reference port, the Arduino development kit and the BEAR Module 9704 documentation set.
