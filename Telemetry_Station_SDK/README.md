# Telemetry Station SDK

Firmware reference in C99 for sending messages over Iridium with the **BEAR Module 9704**. It is small, has no dynamic memory, and does not depend on any one microcontroller. You supply a short platform file for your MCU, and the SDK does the rest.

> **Status: alpha (version 0.2).** The driver has a complete test suite on a computer. Validation on the BEAR Module 9704 and the MSP430F169 reference port is next on the [roadmap](../BEAR_Module/ROADMAP.md). As with any new peripheral, plan bench time for your own board. Contact [telemetrystation.com/contact](https://telemetrystation.com/contact) for questions.

**New here?** Read [Getting started](docs/GETTING_STARTED.md). It explains the terms and gets you to a working demo with no hardware. Firmware reviewers: see the [Design notes](docs/DESIGN.md).

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

## Quick start (any MCU)

```c
#include "ts_bear9704.h"

static void on_event(void *user, ts_event_t event, int32_t detail) {
    /* READY, MO_DELIVERED, IMEI ... see include/ts_bear9704.h */
}

ts_bear9704_t bear;
ts_platform_t port = my_mcu_platform();   /* you write this, see docs/PORTING.md */

ts_bear_init(&bear, &port, on_event, &bear);
ts_bear_start(&bear);

for (;;) {
    ts_bear_poll(&bear);                  /* call every few milliseconds */
    if (done) ts_bear_shutdown(&bear);    /* then remove power after TS_EVENT_STOPPED */
    if (time_to_report && ts_bear_is_ready(&bear))
        ts_bear_send(&bear, TS_TOPIC_ID, bytes, length);
}
```

Run it without hardware: `cmake -S . -B build && cmake --build build && ./build/simulated_demo`.

A complete duty-cycled example for the MSP430F169 (module on only while there is work) is in [`examples/msp430f169/main.c`](examples/msp430f169/main.c).

## Layout

| Path                                     | Purpose                                                                                                                                                                                                                                              |
| ---------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `include/`                               | Public headers: driver, platform interface, config, activation help                                                                                                                                                                                  |
| `src/`                                   | Driver split by job: `ts_bear9704.c` (state machine), `ts_setup.c` (startup), `ts_mo.c` (send), `ts_mt.c` (receive), `ts_cmd.c` (command builder), plus protocol helpers. `ts_names.c` (log names) and `ts_activation.c` (console text) are optional |
| `ports/msp430f169/`                      | MSP430F169 platform file and console                                                                                                                                                                                                                 |
| `ports/template/`                        | Starting point for a new MCU. Compiles as-is                                                                                                                                                                                                         |
| `ports/posix/`                           | Linux/macOS platform for a bench modem on a USB serial adapter                                                                                                                                                                                       |
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

Defaults are sized for the MSP430F169's 2 KB of RAM. The driver's state is 800 bytes with the defaults (728 without receive support), measured on a computer. See [Design notes](docs/DESIGN.md#memory).

## Build and test on a computer

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Add `-DTS_ENABLE_MT=OFF` to test the send-only build. For the strictest check, add `-DTS_WERROR=ON -DTS_SANITIZE=ON` (warnings as errors, AddressSanitizer and UBSan).

`sh tests/coverage.sh` runs the suite with coverage and fails if any file in `src/` has an untested line (use `-DTS_ENABLE_MT=OFF` for the send-only build). The build also compiles the MSP430F169 port and example against a host stand-in for `msp430.h` (`tests/stubs/`), so register and type errors show up without the TI toolchain. The stand-in is for syntax checking only and is not for real targets.

## Things to know before you deploy

- A message reported as **queued** is not **delivered**. Wait for `TS_EVENT_MO_DELIVERED`.
- The SDK never retries a send on its own. A retry after an unknown result can send, and bill, the message twice. Decide your policy in your application.
- Include a sequence number in your payload so duplicates can be spotted.
- Messages carry a 2-byte CRC16 to catch corruption. It is not security and does not prove who sent a message.
- Do not copy another project's topic ID. Use the one Telemetry Station gives you.
- Your event callback runs inside `ts_bear_poll()`. Keep it short. It may read state (`ts_bear_imei`, `ts_bear_state`), but set a flag and call `ts_bear_start`, `ts_bear_send` or `ts_bear_shutdown` from your main loop, as the MSP430 example does.

## License

See [LICENSE](../LICENSE). Contributions: [CONTRIBUTING.md](../CONTRIBUTING.md). Security issues: [SECURITY.md](../SECURITY.md).
