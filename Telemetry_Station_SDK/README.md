# Telemetry Station SDK

This SDK is the software that lets your device send and receive messages through a **BEAR Module 9704**. It handles communication with the modem and reports whether a message was delivered.

**First time? Use [Getting started](docs/GETTING_STARTED.md).** Try the included sample before designing your own message. A real-hardware setup currently requires embedded engineering experience: the F169 is the reference implementation, while other microcontrollers need a hardware adapter written and tested for them.

For developers: the core is portable C99, uses no dynamic memory, and runs through platform callbacks supplied by your MCU adapter.

> **Status: alpha (version 0.2).** The driver has a complete test suite on a computer. Validation on the BEAR Module 9704 and the MSP430F169 reference port is next on the [roadmap](../BEAR_Module/ROADMAP.md). As with any new peripheral, plan bench time for your own board. Contact [telemetrystation.com/contact](https://telemetrystation.com/contact) for questions.

Firmware reviewers: see the [design notes](docs/DESIGN.md).

For BEAR hardware status, SDK hardware validation, and target availability, see the [BEAR Module R&D and release checklist](../BEAR_Module/RD_CHECKLIST.md).

## What it does

- Starts the modem and gets it ready to send.
- Sends a message and reports what happened (queued, delivered, failed).
- Receives messages, if you want that (can be compiled out to save RAM).
- Reads the modem's **IMEI** and prints the next steps for Telemetry Station activation.

## Integrate in this order

| Step | Action | You are done when |
|---|---|---|
| 1. See the sample | [Run the computer demo](docs/GETTING_STARTED.md#2-see-it-work-with-no-hardware) | One simulated send ends with a decoded temperature and shutdown |
| 2. Select your host | [Check support](docs/GETTING_STARTED.md#3-pick-your-path) | Your board has an implemented adapter and reviewed electrical connection |
| 3. Check the module | [Run the engineering checkout](docs/GETTING_STARTED.md#4-check-a-bear-module-on-an-msp430f169) | Real IMEI is readable and shutdown passes, with topic zero |
| 4. Send once | [Activate and send the sample](docs/GETTING_STARTED.md#send-the-included-sample-before-adding-your-sensor) | Network delivery and application receipt are both confirmed |
| 5. Add your sensor | [Customize the sample](docs/GETTING_STARTED.md#5-put-it-in-your-project) | Your actual reading decodes correctly at the destination |

The driver needs no service account to build, simulate or read an IMEI. Real satellite messaging requires activated service. Telemetry Station is the documented service path; the driver can be integrated with another service.

## Developer API sketch (requires a completed MCU adapter)

Use the [complete host demo](examples/host/simulated_demo.c) to learn the API and the [F169 reference application](examples/msp430f169/main.c) for the hardware lifecycle. Start with a working example; do not substitute a few send calls for the startup and shutdown procedure.

The application initializes the driver, starts it, polls every 1 ms in the reference configuration, submits a payload when `ts_bear_can_send()` is true, waits for delivery, and requests shutdown. Only `TS_EVENT_STOPPED` confirms the shutdown handshake. Errors require continued polling and the documented recovery policy.

The F169 bench order and default CCS project run **one checkout per startup**. Topic zero reads the module ID without sending. With a valid assigned topic, that checkout sends one sample. Repeats require an explicit positive interval; see [configuration](#configuration).

## Layout

| Path                                     | Purpose                                                                                                                                                                                                                                              |
| ---------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `include/`                               | Public headers: driver, platform interface, config, activation help                                                                                                                                                                                  |
| `src/`                                   | Driver split by job: `ts_bear9704.c` (state machine), `ts_setup.c` (startup), `ts_mo.c` (send), `ts_mt.c` (receive), `ts_cmd.c` (command builder), plus protocol helpers. `ts_names.c` (log names) and `ts_activation.c` (console text) are optional |
| `ports/msp430f169/`                      | MSP430F169 platform file and console                                                                                                                                                                                                                 |
| `ports/template/`                        | Starting point for a new MCU. Compiles as-is                                                                                                                                                                                                         |
| `ports/posix/`                           | Linux/macOS UART transport skeleton; GPIO lifecycle integration is required                                                                                                                                                                                       |
| `examples/common/`                       | Sensor binding and sample codec; change the matching header/schema when fields change                                                                                                                                                                                          |
| `examples/msp430f169/`, `examples/host/` | Complete example programs, including `simulated_demo`                                                                                                                                                                                                |
| `tests/`                                 | Unit tests and the simulated modem (`sim_modem.c`)                                                                                                                                                                                                   |
| `docs/GETTING_STARTED.md`                | Zero-context walkthrough, glossary, troubleshooting                                                                                                                                                                                                  |
| `docs/PORTING.md`                        | How to add a new microcontroller                                                                                                                                                                                                                     |
| `docs/DESIGN.md`                         | Architecture, timing, memory, error handling, testing                                                                                                                                                                                                |

## Configuration

For the Python F169 build, edit a local copy of `orders/bench.json` and pass it with `--order`. For CCS, use the project compiler definitions. Do not publish your assigned topic. All source files must use the same configuration. Core defaults are in [`include/ts_config.h`](include/ts_config.h).

| Setting                  | Default | Meaning                                                                         |
| ------------------------ | ------- | ------------------------------------------------------------------------------- |
| `TS_TOPIC_ID`            | `0`     | Your topic from Telemetry Station (64 or higher). Sending is blocked until set. |
| `TS_MAX_PAYLOAD`         | `64`    | Largest message in bytes. Use your bytes per message.                           |
| `TS_ENABLE_MT`           | `1`     | Set `0` if you never receive messages, to save RAM.                             |
| `TS_LINE_CAP`            | `256`   | Modem line buffer size.                                                         |
| `TS_QUERY_IMEI_ON_READY` | `1` | Read the IMEI as soon as the modem is ready. |
| `REPORT_INTERVAL_MS` | `0` | F169 application only: one checkout. A positive value opts into repeated cycles after that many idle milliseconds; Python order key: `report_interval_ms`. |

Defaults are sized for the MSP430F169's 2 KB of RAM. The driver's state is 808 bytes with the defaults (736 without receive support), measured on a computer. See [Design notes](docs/DESIGN.md#memory).

## Build and test on a computer

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Add `-DTS_ENABLE_MT=OFF` to test the send-only build. For the strictest check, add `-DTS_WERROR=ON -DTS_SANITIZE=ON` (warnings as errors, AddressSanitizer and UBSan).

Add `-DTS_QUERY_IMEI_ON_READY=OFF` to have the application request the IMEI instead of the driver.

`python3 tests/coverage.py` runs the suite with coverage and fails unless every line and branch of `src/`, the checkout and the F169 port and firmware is tested (unreachable branches are waived with a reason in `tests/branch_waivers.json`). Run `sh tests/coverage.sh -DTS_ENABLE_MT=OFF` to check the send-only build too; CI checks both MT configurations independently, without cancelling one when the other fails. `python3 tests/run_matrix.py` runs every feature combination under sanitizers. The build also compiles the MSP430F169 port and example against a host stand-in for `msp430.h` (`tests/stubs/`), so register and type errors show up without the TI toolchain. The stand-in also drives host register-model tests; it does not establish real electrical or timing behavior and must never be used in a target build.

Activation regressions compare the complete guide for unknown (zero), typical,
and `UINT_MAX` payload sizes, including every undersized buffer and an exact fit
with its terminator. GNSS regressions reject malformed bytes at each of the six
UBX header positions and recover a valid fix, including when a rejected byte is
the start of an overlapping sync sequence.

## Fault checks behind the sample

The driver checks its serial adapter's byte counts and rejects lines containing an embedded NUL instead of accepting a damaged reply's prefix. Regression tests also verify recovery at the next complete line. These protections preserve the same setup, send and shutdown API; they do not replace hardware validation. See the [engineering evidence](docs/ENGINEERING.md#serial-and-configuration-checks).

## Things to know before you deploy

- A message reported as **queued** is not **delivered**. Wait for `TS_EVENT_MO_DELIVERED`.
- The SDK never retries a send on its own. A retry after an unknown result can send, and bill, the message twice. Decide your policy in your application.
- Include a sequence number in your payload so duplicates can be spotted.
- Messages carry a 2-byte CRC16 to catch corruption. It is not security and does not prove who sent a message.
- Do not copy another project's topic ID. Use the one Telemetry Station gives you.
- Your event callback runs inside `ts_bear_poll()`. Keep it short. It may read state (`ts_bear_imei`, `ts_bear_state`), but set a flag and call `ts_bear_start`, `ts_bear_send` or `ts_bear_shutdown` from your main loop, as the MSP430 example does.

## License

See [LICENSE](../LICENSE). Contributions: [CONTRIBUTING.md](../CONTRIBUTING.md). Security issues: [SECURITY.md](../SECURITY.md).

## Engineering baseline — 2026-11-03

See [Engineering baseline](docs/ENGINEERING.md) for validated feature orders,
actual TI F169 builds, the generated Certus Payload Optimizer codec, optional
LEA-M8S acquisition, lifecycle fixes, coverage and release-record checks.
See [hardware validation](tests/hardware/README.md) for CCS capture and bench
procedures. With a BEAR Module wired to an MSP430F169, flash the bench image and
open USART0 at 115200 for a PASS/CHECK checkout of every step; see
[Getting started](docs/GETTING_STARTED.md#4-check-a-bear-module-on-an-msp430f169). These additions do not qualify BEAR hardware for production.

**Next:** [run the included sample](docs/GETTING_STARTED.md#2-see-it-work-with-no-hardware), then choose the hardware path that matches your board.
