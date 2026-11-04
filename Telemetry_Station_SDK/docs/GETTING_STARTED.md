# Getting started

This guide assumes you know embedded C and UARTs, and nothing about Iridium, Telemetry Station or the BEAR Module. About 30 minutes to run the demo, and an afternoon to get a real message through.

## 1. What problem this solves

Your sensor is somewhere with no cellular or Wi-Fi coverage. You want it to send a small reading, a few bytes every few minutes or hours, to a server you can query. The Iridium satellite network covers the whole Earth, so a satellite modem on your board can do that.

Talking to an Iridium modem takes real work: a serial command protocol, a careful power-up and power-down sequence, message segmenting, delivery tracking. This SDK does that part in portable C so your application only says "send these bytes" and gets "delivered" or "failed" back.

```text
your sensor --> your MCU + this SDK --> BEAR Module 9704 --> Iridium satellites --> cloud
```

### Words you will meet

| Word | Meaning |
|---|---|
| **BEAR Module 9704** | The hardware: an Iridium 9704 modem plus power management on one board. In development. See [`BEAR_Module/`](../../BEAR_Module/README.md) |
| **Iridium 9704** | The satellite modem chip module on the board |
| **IMEI** | A 15-digit serial number unique to each modem. You use it to activate the modem for service |
| **Activation** | Registering the IMEI so the network will carry your messages. Done on telemetrystation.com |
| **Topic ID** | A number, 64 or higher, that identifies where your messages go. You receive it at activation |
| **Payload** | The bytes your device sends. Smaller means cheaper, so pack them tightly |
| **Payload Optimizer** | Free web tool that designs a compact payload and generates `payload.c` and `payload.h` |
| **MO / MT** | Mobile-originated (device to cloud) and mobile-terminated (cloud to device) |
| **Power management** | The module's supply circuitry. See below |

**Power management, briefly.** A satellite modem idles on little power but pulls large, short current bursts when it transmits. Many small supplies cannot provide those bursts and will brown out. The module smooths this out and switches the modem on and off in the safe order. You only drive an `ENABLE` pin and watch a `BOOTED` pin.

## 2. See it work with no hardware

You need CMake and a C compiler.

```sh
cd Telemetry_Station_SDK
cmake -S . -B build
cmake --build build
./build/simulated_demo
```

You should see:

```text
[    48 ms] TS_EVENT_READY         detail=0
[    53 ms] TS_EVENT_IMEI          detail=0
Modem IMEI: 300000000000001
1. Size your plan: https://telemetrystation.com/calculators/certus?payloadBytes=4
2. Activate: https://telemetrystation.com/activate/certus
[    81 ms] TS_EVENT_MO_ACCEPTED   detail=9
[   122 ms] TS_EVENT_MO_DELIVERED  detail=9
[   137 ms] TS_EVENT_STOPPED       detail=0
BOOTED is low: it is now safe to remove power from the module.
```

That is the whole life of one message: start, ready, IMEI, send, delivered, shut down. The modem here is simulated ([`tests/sim_modem.c`](../tests/sim_modem.c)); the driver is the real one. Read [`examples/host/simulated_demo.c`](../examples/host/simulated_demo.c), about 80 lines, to see the API in use.

## 3. Pick your path

| You have | Do this |
|---|---|
| Nothing yet, just evaluating | Stop at step 2, then read [Design notes](DESIGN.md) |
| A TI MSP430F169 and a BEAR Module | Run the checkout in step 4, then step 5 with `ports/msp430f169`. Module availability: see the [roadmap](../../BEAR_Module/ROADMAP.md) |
| Another microcontroller | Follow [Porting](PORTING.md). You write seven small functions, starting from `ports/template` |
| A computer and a modem on a USB serial adapter | Use `ports/posix` and the `ts_posix_modem` program |

## 4. Check a BEAR Module on an MSP430F169 (flash and go)

The F169 example is a self-test. Flash it, open a terminal, and it tells you whether every part of the connection works. You do not need an account or a topic ID.

1. **Wire it.**

   | F169 pin | Connects to |
   |---|---|
   | P1.0 | BEAR `ENABLE` |
   | P1.1 | BEAR `BOOTED` |
   | P3.6 (USART1 TX) | BEAR `RXD` |
   | P3.7 (USART1 RX) | BEAR `TXD` |
   | P3.4 (USART0 TX) | USB serial adapter RX (console) |
   | GND | BEAR ground and adapter ground |

   You also need an 8 MHz crystal on XT2. Check logic levels on both sides before you power up.
2. **Build and flash.** In CCS, import [`ccs/bear9704_sdk_msp430f169.projectspec`](../ccs/bear9704_sdk_msp430f169.projectspec), build, and flash. From the command line, `python3 tools/build_f169.py --order orders/bench.json` builds the same image (the console is on in this order).
3. **Open the console** at **115200 8N1** on the adapter. Any terminal works; colour is ANSI.
4. **Reset the board.** One checkout cycle runs immediately, then one every 10 minutes (`report_interval_ms`).

A healthy module looks like this:

```text
============================================================
 Telemetry Station SDK v0.2.0  |  BEAR Module 9704 checkout
============================================================
 Board    MSP430F169, XT2 8 MHz, watchdog 0
 Wiring   ENABLE P1.0, BOOTED P1.1, UART1 TX P3.6 / RX P3.7
 Console  USART0 TX P3.4, 115200 8N1; modem 230400 8N1
 Topic    not set; send test skipped
 Options  receive on, payload max 64 B, cycle every 600 s
------------------------------------------------------------
[    0.036] PASS   Module off at reset  BOOTED low
[    1.501] PASS   Module boot          BOOTED high after 1501 ms
[    1.504] PASS   Serial link          modem replied in 3 ms
[    1.511] PASS   SIM                  internal SIM selected
[    1.519] PASS   Radio active         operational state active
[    1.522] PASS   IMEI                 300000000000001
[    1.527] PASS   Satellite signal     4/5 bars
[    1.534] SKIP   Message queued       TS_TOPIC_ID not set
[    1.541] SKIP   Delivery             nothing sent
[    2.233] PASS   Power down           BOOTED low after 706 ms; safe to cut power
------------------------------------------------------------
 RESULT   PASS   8 pass  0 warn  0 check  2 skip   2.2 s
------------------------------------------------------------
```

(Times are from the simulated modem; a real module takes longer to boot and to find the sky.) The IMEI is printed with the activation links. Once you have a topic ID, set `TS_TOPIC_ID` (or `topic_id` in the order) and the send and delivery rows run too. Later cycles print a one-line summary.

**RESULT** is PASS when every step passed, WARN when something works but needs attention (for example 0 bars indoors), and CHECK when a step needs your attention before the module can be used. A CHECK step marks the steps that depend on it as SKIP, and the module is always powered down before the cycle ends.

| Row shows | What to check |
|---|---|
| Nothing at all on the console | Console wiring (P3.4 to adapter RX, common ground) and baud. If still blank, XT2: the firmware stops with everything off when the crystal does not start |
| Module off at reset CHECK `BOOTED stuck high` | `BOOTED` wiring or a module still powered from an earlier run |
| Module boot CHECK `BOOTED never rose` | Module supply, `ENABLE` wiring. The driver then holds `ENABLE` for safety, so Power down also reports `ENABLE held`; power-cycle after fixing it |
| Serial link CHECK `no reply; check TX/RX crossover and baud` | TX and RX swapped or unconnected |
| SIM / Radio active / IMEI CHECK | The modem answered with an error: note the detail and the IMEI, and report it |
| Satellite signal WARN `antenna needs open sky` | Normal indoors. Go outside with a clear view of the sky |
| Delivery CHECK `not confirmed in time; outcome unknown` | Sky view, or topic not activated. Queued is not delivered; do not resend automatically |
| Power down CHECK `BOOTED still high` / `BOOTED low, ENABLE held` | The module did not stop cleanly. Power-cycle the board and check `BOOTED` |

You can see every outcome without hardware: `build/test_checkout transcript 1234` prints a full run, and `build/test_f169_firmware_0 console nomodule` (or `swapped`) runs the real firmware on an emulated F169 with a missing module or crossed wires.

## 5. Put it in your project

1. **Design your message.** Open the [Payload Optimizer](https://telemetrystation.com/payload-optimizer), describe your readings, and download `payload.c` and `payload.h`. Note the **bytes per message**.
2. **Connect your encoder.** Edit [`examples/common/app_payload.c`](../examples/common/app_payload.c) to call the generated code. Set `TS_MAX_PAYLOAD` to your bytes per message.
3. **Write or choose a port.** Seven functions in a `ts_platform_t` struct connect the SDK to your UART, a millisecond clock, and two GPIO pins.
4. **Wire the module.** `ENABLE` (output from your MCU), `BOOTED` (input), UART `TXD`/`RXD` crossed, common ground. Check logic voltages on both sides before powering. The F169 wiring is in the [integration guide](../../BEAR_Module/docs/06_Integration_Guide.md).
5. **Run it.** The firmware prints the **IMEI** and the next steps. You do not need a topic ID for this.
6. **Activate.** Put your bytes per message into the [calculator](https://telemetrystation.com/calculators/certus), then [activate](https://telemetrystation.com/activate/certus) the IMEI. You get a **topic ID**.
7. **Set `TS_TOPIC_ID`**, rebuild, go outdoors with a clear view of the sky, and send.
8. **Wait for `TS_EVENT_MO_DELIVERED`.** Queued is not delivered.

The main loop is short:

```c
ts_bear_poll(&bear);                      /* every few ms: does all the work */
if (ts_bear_is_ready(&bear) && have_data)
    ts_bear_send(&bear, TS_TOPIC_ID, bytes, length);
```

For a battery product, start the module only when you have something to send and shut it down after delivery. [`examples/msp430f169/main.c`](../examples/msp430f169/main.c) shows the pattern as a small state machine.

## 6. When it does not work

| Symptom | Likely cause |
|---|---|
| `ts_bear_init` returns `TS_ERR_ARGUMENT` | A function is missing from your `ts_platform_t` |
| Stays in `TS_BOOTING`, then `TS_EVENT_ERROR` / `TS_ERR_TIMEOUT` | `BOOTED` never rose: check `ENABLE`, supply and your `boot_ready()` polarity |
| Boots but no IMEI | UART TX and RX swapped, or baud rate wrong. It must be 230400 8N1 and needs a crystal-based clock on many MCUs |
| `ts_bear_send` returns `TS_ERR_NOT_READY` | Still starting up, or the IMEI query is answering. Try again on the next loop |
| `ts_bear_send` returns `TS_ERR_CAPACITY` | Topic below 64, or payload bigger than `TS_MAX_PAYLOAD` |
| `TS_EVENT_MO_ACCEPTED` but never delivered | No view of the sky, or the modem is not activated for that topic. Check the sky first |
| `ts_bear_start` returns `TS_ERR_BUSY` | The module is still shutting down. Wait for `TS_EVENT_STOPPED` |
| Occasional corrupted or lost bytes | Your receive ring buffer is too small for your loop period |

Print names instead of numbers while debugging: add `src/ts_names.c` and use `ts_event_name()`, `ts_state_name()` and `ts_result_name()`.

## 7. Questions you probably have

**Do I have to use Telemetry Station?** No. The driver talks to the modem and needs no account. Telemetry Station is the path we build the hardware and data service around.

**Can I buy the hardware today?** The BEAR Module 9704 is in development. See the [roadmap](../../BEAR_Module/ROADMAP.md) for product status and target availability.

**Is it ready for production?** It is alpha, version 0.2, and the hardware is in development. Use it now to evaluate and prototype, and see [Status and roadmap](../../README.md#status-and-roadmap) for what comes next.

**How big is it?** About 0.8 KB of RAM with defaults in the host build. The complete F169 checkout firmware (driver, checkout, console) uses about 18.9 KB of flash and 2025 of 2048 bytes of RAM including the 512-byte stack; the build gates a worst-case stack of 384 bytes. See [Design notes](DESIGN.md#memory) and the [engineering baseline](ENGINEERING.md).

**Will it retry a failed send?** No, on purpose. A retry after an unknown result can double-send. You decide.

**Is it secure?** The SDK checks integrity with a CRC. It does not encrypt or authenticate. Encrypt inside your payload if you need to.

**Where do I ask questions?** Open an issue on GitHub.

### 2026-11-03 engineering revision

The [engineering baseline](ENGINEERING.md) supersedes earlier examples where they
show a placeholder payload, imply that every FAULT is powered down, assume
unbounded clock/console waits, or imply a target build was not performed.
`TS_FAULT_HOLD` needs continued polling and reviewed hardware recovery; restart
also checks BOOTED. `ts_f169_init()` returns success/failure. The bench order and CCS
project enable the checkout console; the other orders leave it off. Follow the new order/build and hardware
validation procedures before adapting this bench example to a product.
