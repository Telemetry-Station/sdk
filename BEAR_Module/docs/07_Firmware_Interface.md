# 07 Firmware Interface

Document ID: BEAR-FW-9704 | Revision: P1 | Status: Preliminary

The BEAR Module 9704 is controlled by the [Telemetry Station SDK](../../Telemetry_Station_SDK), a portable C99 library. This document maps the module's hardware signals to the SDK, so hardware and firmware engineers can work from one reference.

## Signal to SDK mapping

| Module signal | SDK platform function (`ts_platform_t`) | What the SDK does |
|---|---|---|
| ENABLE | `power_start()`, `power_stop()` | Drives ENABLE high to start, low to stop |
| BOOTED | `boot_ready()` | Waits for high before enabling the UART. After shutdown, waits for low before allowing a restart |
| TXD, RXD | `serial_read()`, `serial_write()`, `uart_enable()` | Never blocks. Disables the UART and tri-states its pins when the module is off |
| (timing) | `millis()` | Drives all timeouts. Wrap-around is handled |
| TX_ACTIVE, WAKE_IN, WAKE_OUT | Not used by the SDK today | Available to your application |

## Driver states

| SDK state | Hardware condition |
|---|---|
| `TS_OFF` | ENABLE low, BOOTED low |
| `TS_BOOTING` | ENABLE high, waiting for BOOTED high |
| `TS_API_QUERY` to `TS_MODE_SET` | UART handshake: API version, SIM selection, operational state |
| `TS_READY` | Modem active, may send |
| `TS_MO_START`, `TS_MO_ACTIVE` | A message is being handed to the modem |
| `TS_SHUTDOWN_WAIT` | Modem is being commanded inactive |
| `TS_POWERING_DOWN` | ENABLE low, waiting for BOOTED low |
| `TS_FAULT` | Error. The module has been powered down. Call `ts_bear_start()` to retry |

Read the state with `ts_bear_state()`. `TS_OFF` and `TS_FAULT` both mean the module is powered down.

## Events

| Event | Meaning | Hardware or workflow relevance |
|---|---|---|
| `TS_EVENT_READY` | Handshake complete | Module is registered for service |
| `TS_EVENT_IMEI` | IMEI available via `ts_bear_imei()` | Print with `ts_activation_text()` to start activation |
| `TS_EVENT_SIGNAL` | Signal bars available | Commissioning and antenna checks |
| `TS_EVENT_MO_ACCEPTED` | Modem queued the message | Not delivery |
| `TS_EVENT_MO_DELIVERED` | Iridium confirmed receipt | Safe to shut down the module |
| `TS_EVENT_MO_FAILED` | Send failed. The outcome may be unknown | Application decides whether to resend |
| `TS_EVENT_MT_RECEIVED`, `TS_EVENT_MT_DROPPED` | Incoming message arrived or was rejected | Optional receive support |
| `TS_EVENT_STOPPED` | Shutdown handshake complete | Safe to remove VIN |
| `TS_EVENT_ERROR` | Fault | Check `detail` for the cause |

## Typical duty-cycled application

The module is the largest consumer on the board, so a battery product starts it only when there is something to send. The whole pattern is a three-state loop, implemented in [`examples/msp430f169/main.c`](../../Telemetry_Station_SDK/examples/msp430f169/main.c):

```text
SLEEPING --(report interval elapsed)--> WORKING --(delivered or failed)--> FINISHING --(TS_OFF)--> SLEEPING
 ts_bear_start()                          ts_bear_send()                    ts_bear_shutdown()
```

```c
ts_bear_poll(&bear);                         /* every few ms: does all the work */
switch (ts_bear_state(&bear)) {
case TS_READY:  /* send once, then wait for TS_EVENT_MO_DELIVERED */ break;
case TS_OFF:    /* powered down: VIN may now be removed */            break;
case TS_FAULT:  /* driver powered the module down; retry next interval */ break;
default:        break;
}
```

Run the same sequence with no hardware: [`simulated_demo`](../../Telemetry_Station_SDK/examples/host/simulated_demo.c).

## IMEI and activation

When the modem reaches `TS_READY` the SDK queries its IMEI (`TS_QUERY_IMEI_ON_READY`). `ts_activation_text()` formats it with the calculator and activation links. The IMEI is also planned to appear on the module label, so activation can start before your firmware is complete.

## Configuration

| SDK setting | Hardware relevance |
|---|---|
| `TS_TOPIC_ID` | Topic issued at activation |
| `TS_MAX_PAYLOAD` | Bytes per message from the Payload Optimizer. Sizes RAM buffers |
| `TS_BOOT_TIMEOUT_MS` | Upper limit for t_BOOT and t_DOWN. Default 30 s |
| `TS_ENABLE_MT` | Receive support. Disable it to save RAM |
| `TS_LINE_CAP` | Serial line buffer size |
| `TS_F169_WATCHDOG` | F169 port only. Enables the hardware watchdog (needs a 32.768 kHz crystal) |

Defaults are sized for 2 KB of RAM (MSP430F169). The driver's state is 800 bytes with the defaults, measured on a computer. See the [design notes](../../Telemetry_Station_SDK/docs/DESIGN.md).

## Porting

To use another microcontroller, copy [`ports/template`](../../Telemetry_Station_SDK/ports/template/ts_port_template.c) and implement the seven functions of `ts_platform_t`. The [porting guide](../../Telemetry_Station_SDK/docs/PORTING.md) lists the requirements. Reference ports:

| Port | Status |
|---|---|
| TI MSP430F169 | Reference port. Bench bring-up with the BEAR Module 9704 is on the [roadmap](../ROADMAP.md) |
| Linux and macOS with a USB-serial adapter | Bench use with an already-powered module |
| Additional MCUs | Planned. Contributions welcome |

## Validation status

| Item | Status |
|---|---|
| Driver against a simulated modem | Passing in CI, with strict warnings and sanitizers |
| Driver against BEAR Module EVT hardware | Coming soon, at G2 |
| F169 port on target | Coming soon, at G2 |
