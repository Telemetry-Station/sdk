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
Modem IMEI: 300234010753370
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
| An Arduino and the Iridium 9704 LaunchPad | Use the [Iridium Development Kit](../../Iridium_Development_Kit_9704/README.md) to send test messages today |
| A TI MSP430F169 and a BEAR Module | Follow step 4 with `ports/msp430f169`. Module availability: see the [roadmap](../../BEAR_Module/ROADMAP.md) |
| Another microcontroller | Follow [Porting](PORTING.md). You write seven small functions, starting from `ports/template` |
| A computer and a modem on a USB serial adapter | Use `ports/posix` and the `ts_posix_modem` program |

## 4. Put it in your project

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

## 5. When it does not work

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

## 6. Questions you probably have

**Do I have to use Telemetry Station?** No. The driver talks to the modem and needs no account. Telemetry Station is the path we build the hardware and data service around.

**Can I buy the hardware today?** The BEAR Module 9704 is in development. See the [roadmap](../../BEAR_Module/ROADMAP.md). You can develop against the Iridium 9704 LaunchPad now.

**Is it ready for production?** It is alpha, version 0.2, and the hardware is in development. Use it now to evaluate and prototype, and see [Status and roadmap](../../README.md#status-and-roadmap) for what comes next.

**How big is it?** About 0.8 KB of RAM with defaults. Flash figures are coming soon. See [Design notes](DESIGN.md#memory).

**Will it retry a failed send?** No, on purpose. A retry after an unknown result can double-send. You decide.

**Is it secure?** The SDK checks integrity with a CRC. It does not encrypt or authenticate. Encrypt inside your payload if you need to.

**Where do I ask questions?** Open an issue on GitHub.
