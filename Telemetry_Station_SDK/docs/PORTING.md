# Porting to a new microcontroller

The driver never touches hardware directly. It calls the functions in a `ts_platform_t` ([`include/ts_platform.h`](../include/ts_platform.h)). To support a new MCU, fill in that struct. Start from [`ports/template/ts_port_template.c`](../ports/template/ts_port_template.c), which compiles as-is and has a TODO at each step. The MSP430F169 version in [`ports/msp430f169/`](../ports/msp430f169/ts_f169_port.c) is a worked example.

## What you provide

| Function | What it does |
|---|---|
| `serial_read` | Return the number of bytes stored (the driver asks for one), `0` if none, negative on error (such as lost bytes). Returns `int16_t`. Never block. |
| `serial_write` | Accept up to `length` bytes and return how many were taken (`int16_t`). May take fewer. Never block. |
| `millis` | Milliseconds since start. Wraparound is handled. |
| `power_start` / `power_stop` | Turn the BEAR Module on and off. |
| `uart_enable` | Enable the UART, or disable it and make its pins inputs so the module is not back-powered. |
| `boot_ready` | True once the module reports it has booted. |

Use interrupts to fill a receive buffer. At 230400 baud a byte arrives about every 43 microseconds, so polling a one-byte register will lose data.

## Serial settings

The BEAR Module 9704 talks 8N1 at 230400 baud. Check your MCU's clock can produce this accurately. On the MSP430F169 the internal oscillator cannot, so the port uses an external 8 MHz crystal.

## What "power management" means

The Iridium modem draws very little power while idle and short, large bursts of current when it transmits. A bare microcontroller supply usually cannot supply those bursts and may reset. The BEAR Module includes **power management**: circuitry that regulates the supply to the modem, copes with the bursts, and powers it on and off in a safe order. Your firmware only needs to drive an enable line and watch a "booted" signal. These are the `power_start`, `power_stop` and `boot_ready` functions above.

The modem needs a complete shutdown handshake: after `power_stop`, wait for `boot_ready` to read low before restarting or removing main power. The driver does this and reports `TS_EVENT_STOPPED` when it is safe.

Hardware details are in the [BEAR Module documentation](../../BEAR_Module/README.md). The exact pins and logic levels of the BEAR Module 9704 are published at Rev A. The pins in the F169 port are bench wiring. Confirm voltage levels on both sides before connecting.

## Checklist

1. Copy `ports/template/ts_port_template.c` and fill in the seven functions.
2. Add the `src/` files to your build (`ts_names.c` and `ts_activation.c` are optional) and `include/` to your include path.
3. Call `ts_bear_init`, `ts_bear_start`, then `ts_bear_poll` in your main loop.
4. Size your UART receive buffer for your loop period: at 230400 baud a byte arrives every 43 microseconds.
5. Connect to the module and watch for `TS_EVENT_READY`, then `TS_EVENT_IMEI`.
6. Send a message only after you have activated the modem and set `TS_TOPIC_ID`.
7. Before deployment, add a hardware watchdog kicked once per main-loop pass, and decide what your product does after `TS_FAULT`. See [Design notes](DESIGN.md#watchdog).

Run the host tests (`ctest`) with your compiler's warnings enabled to confirm the portable code builds cleanly. They do not test your port.
