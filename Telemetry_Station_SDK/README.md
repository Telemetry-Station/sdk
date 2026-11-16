# Telemetry Station SDK

This SDK is the software that lets your device send and receive messages through a **BEAR Module 9704**. It handles communication with the modem and reports whether a message was delivered.

**First time? Use [Getting started](docs/GETTING_STARTED.md).** Try the included sample before designing your own message. A real-hardware setup currently requires embedded engineering experience: the F169 is the reference implementation, while other microcontrollers need a hardware adapter written and tested for them.

For developers: the core is portable C99, uses no dynamic memory, and runs through platform callbacks supplied by your MCU adapter.

> **Status: alpha (version 0.2).** The driver has a complete test suite on a computer. Validation on the BEAR Module 9704 and the MSP430F169 reference port is next on the [roadmap](../BEAR_Module/ROADMAP.md). As with any new peripheral, plan bench time for your own board. Contact [telemetrystation.com/contact](https://telemetrystation.com/contact) for questions.

**New here?** Read [Getting started](docs/GETTING_STARTED.md). It explains the terms and gets you to a working demo with no hardware. Firmware reviewers: see the [Design notes](docs/DESIGN.md).

For BEAR hardware status, SDK hardware validation, and target availability, see the [BEAR Module R&D and release checklist](../BEAR_Module/RD_CHECKLIST.md).

## What it does

- Starts the modem and gets it ready to send.
- Sends a message and reports what happened (queued, delivered, failed).
- Receives messages, if you want that (can be compiled out to save RAM).
- Reads the modem's **IMEI** and prints the next steps for Telemetry Station activation.

## The Telemetry Station workflow

1. Open the [Payload Optimizer](https://telemetrystation.com/payload-optimizer), describe your data, and download `payload.c` and `payload.h`. They encode (and decode) your readings into the fewest bytes. The tool also tells you your **bytes per message**.
2. Put those files in your project. Connect them to the SDK in [`examples/common/app_payload.c`](examples/common/app_payload.c).
3. Set `TS_MAX_PAYLOAD` to your bytes per message (see [Configuration](#configuration)).
4. Run the firmware. When the modem is ready, it shows its **IMEI** and links to the next steps.
5. Enter your project's needs and the bytes per message in the [calculator](https://telemetrystation.com/calculators/certus), then [activate](https://telemetrystation.com/activate/certus) the modem using the IMEI. Telemetry Station gives you the **topic ID**. Set it as `TS_TOPIC_ID`.

Using Telemetry Station is encouraged but optional. The driver talks to the modem and does not require an account.

## Developer API sketch (requires a completed MCU adapter)

This sketch shows the application flow; it is not a complete program to paste into an arbitrary board. For a runnable example, use the simulation or the [F169 engineering setup](docs/GETTING_STARTED.md#4-check-a-bear-module-on-an-msp430f169-flash-and-go).

```c
#include "ts_bear9704.h"

static void on_event(void *user, ts_event_t event, int32_t detail) {
    /* READY, MO_DELIVERED, IMEI ... see include/ts_bear9704.h */
}

ts_bear9704_t bear;
ts_platform_t port = my_mcu_platform();   /* you write this, see docs/PORTING.md */

/* Apply VIN first. Implement port.supply_ready; see the Rev A charge policy. */
if (ts_bear_init(&bear, &port, on_event, &bear) != TS_OK) return;
if (ts_bear_start(&bear) != TS_OK) return;

for (;;) {
    ts_bear_poll(&bear);                  /* call every 1 ms (validate worst-case service latency) */
    if (done) {
        /* Retry BUSY later; remove VIN only after TS_EVENT_STOPPED. */
        (void)ts_bear_shutdown(&bear);
    } else if (time_to_report && ts_bear_can_send(&bear)) {
        ts_result_t result = ts_bear_send(&bear, TS_TOPIC_ID, bytes, length);
        if (result == TS_OK) time_to_report = false; /* Accepted locally. */
        /* Otherwise retain the payload and handle result. Wait for delivery. */
    }
}
```

Run it without hardware: `cmake -S . -B build && cmake --build build && ./build/simulated_demo`.

`TS_EVENT_READY` means protocol initialization is complete; an automatic IMEI query may still be pending. `ts_bear_can_send()` reports whether a valid send can be accepted now. The Rev A supply gate and host electrical requirements are described in [Porting](docs/PORTING.md#rev-a-integration-contract--2026-11-15).

A complete duty-cycled example for the MSP430F169 (module on only while there is work) is in [`examples/msp430f169/main.c`](examples/msp430f169/main.c).

## Layout

| Path                                     | Purpose                                                                                                                                                                                                                                              |
| ---------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `include/`                               | Public headers: driver, platform interface, config, activation help                                                                                                                                                                                  |
| `src/`                                   | Driver split by job: `ts_bear9704.c` (state machine), `ts_setup.c` (startup), `ts_mo.c` (send), `ts_mt.c` (receive), `ts_cmd.c` (command builder), plus protocol helpers. `ts_names.c` (log names) and `ts_activation.c` (console text) are optional |
| `ports/msp430f169/`                      | MSP430F169 platform file and console                                                                                                                                                                                                                 |
| `ports/template/`                        | Starting point for a new MCU. Compiles as-is                                                                                                                                                                                                         |
| `ports/posix/`                           | Linux/macOS UART transport skeleton; GPIO lifecycle integration is required                                                                                                                                                                                       |
| `examples/common/`                       | `app_payload.c`, the one file you edit to match your device                                                                                                                                                                                          |
| `examples/msp430f169/`, `examples/host/` | Complete example programs, including `simulated_demo`                                                                                                                                                                                                |
| `tests/`                                 | Unit tests and the simulated modem (`sim_modem.c`)                                                                                                                                                                                                   |
| `docs/GETTING_STARTED.md`                | Zero-context walkthrough, glossary, troubleshooting                                                                                                                                                                                                  |
| `docs/PORTING.md`                        | How to add a new microcontroller                                                                                                                                                                                                                     |
| `docs/DESIGN.md`                         | Architecture, timing, memory, error handling, testing                                                                                                                                                                                                |

## Configuration

Set these at build time (for example `-DTS_MAX_PAYLOAD=40`) or in [`include/ts_config.h`](include/ts_config.h).

| Setting                  | Default | Meaning                                                                         |
| ------------------------ | ------- | ------------------------------------------------------------------------------- |
| `TS_TOPIC_ID`            | `0`     | Your topic from Telemetry Station (64 or higher). Sending is blocked until set. |
| `TS_MAX_PAYLOAD`         | `64`    | Largest message in bytes. Use your bytes per message.                           |
| `TS_ENABLE_MT`           | `1`     | Set `0` if you never receive messages, to save RAM.                             |
| `TS_LINE_CAP`            | `256`   | Modem line buffer size.                                                         |
| `TS_QUERY_IMEI_ON_READY` | `1`     | Read the IMEI as soon as the modem is ready.                                    |

Defaults are sized for the MSP430F169's 2 KB of RAM. The driver's state is 808 bytes with the defaults (736 without receive support), measured on a computer. See [Design notes](docs/DESIGN.md#memory).

## Build and test on a computer

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Add `-DTS_ENABLE_MT=OFF` to test the send-only build. For the strictest check, add `-DTS_WERROR=ON -DTS_SANITIZE=ON` (warnings as errors, AddressSanitizer and UBSan).

Add `-DTS_QUERY_IMEI_ON_READY=OFF` to have the application request the IMEI instead of the driver.

`python3 tests/coverage.py` runs the suite with coverage and fails unless every line and branch of `src/`, the checkout and the F169 port and firmware is tested (unreachable branches are waived with a reason in `tests/branch_waivers.json`). Run `sh tests/coverage.sh -DTS_ENABLE_MT=OFF` to check the send-only build too; CI checks both MT configurations independently, without cancelling one when the other fails. `python3 tests/run_matrix.py` runs every feature combination under sanitizers. The build also compiles the MSP430F169 port and example against a host stand-in for `msp430.h` (`tests/stubs/`), so register and type errors show up without the TI toolchain. The stand-in is for syntax checking only and is not for real targets.

Activation regressions compare the complete guide for unknown (zero), typical,
and `UINT_MAX` payload sizes, including every undersized buffer and an exact fit
with its terminator. GNSS regressions reject malformed bytes at each of the six
UBX header positions and recover a valid fix, including when a rejected byte is
the start of an overlapping sync sequence.

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
[Getting started](docs/GETTING_STARTED.md#4-check-a-bear-module-on-an-msp430f169-flash-and-go). These additions do not qualify BEAR hardware for production.
