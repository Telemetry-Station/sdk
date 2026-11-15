# Porting to a new microcontroller

The driver never touches hardware directly. It calls the functions in a `ts_platform_t` ([`include/ts_platform.h`](../include/ts_platform.h)). To support a new MCU, fill in that struct. Start from [`ports/template/ts_port_template.c`](../ports/template/ts_port_template.c), which compiles as-is and has a TODO at each step. The MSP430F169 version in [`ports/msp430f169/`](../ports/msp430f169/ts_f169_port.c) is a worked example.

## What you provide

| Function | What it does |
|---|---|
| `serial_read` | Return the number of bytes stored (the driver asks for one), `0` if none, negative on error (such as lost bytes). Returns `int16_t`. Never block. |
| `serial_write` | Accept up to `length` bytes and return how many were taken (`int16_t`). May take fewer. Never block. |
| `millis` | Milliseconds since start. Wraparound is handled. |
| `power_start` / `power_stop` | Assert/deassert ENABLE only; never switch VIN here. |
| `supply_ready` | Nonblocking stored-energy qualification before ENABLE. Template fails closed until implemented. NULL means the application explicitly qualifies supply before every start. |
| `uart_enable` | Enable the UART, or disable it and make its pins inputs so the module is not back-powered. |
| `boot_ready` | True once the module reports it has booted. |

Use interrupts to fill a receive buffer. At 230400 baud a byte arrives about every 43 microseconds, so polling a one-byte register will lose data.

## Serial settings

The BEAR Module 9704 talks 8N1 at 230400 baud. Check your MCU's clock can produce this accurately. On the MSP430F169 the internal oscillator cannot, so the port uses an external 8 MHz crystal.

## What "power management" means

The Iridium modem draws very little power while idle and short, large bursts of current when it transmits. A bare microcontroller supply usually cannot supply those bursts and may reset. The BEAR Module includes **power management**: circuitry that regulates the supply to the modem, copes with the bursts, and provides an ENABLE/BOOTED interface sequenced by the SDK after the host qualifies stored energy. Your firmware only needs to drive an enable line and watch a "booted" signal. These are the `power_start`, `power_stop` and `boot_ready` functions above.

The modem needs a complete shutdown handshake: after `power_stop`, wait for `boot_ready` to read low before restarting or removing main power. The driver does this and reports `TS_EVENT_STOPPED` when it is safe.

Hardware details are in the [BEAR Module documentation](../../BEAR_Module/README.md). The exact pins and logic levels of the BEAR Module 9704 are published at Rev A. The pins in the F169 port are bench wiring. Confirm voltage levels on both sides before connecting.

## Checklist

1. Copy `ports/template/ts_port_template.c` and fill in the seven required callbacks and the supply_ready gate.
2. Add the `src/` files to your build (`ts_names.c` and `ts_activation.c` are optional) and `include/` to your include path.
3. Apply VIN, establish ENABLE low, configure the supply-ready gate, then call `ts_bear_init`, `ts_bear_start` and `ts_bear_poll` every 1 ms in your main loop.
4. Size your UART receive buffer for your loop period: at 230400 baud a byte arrives every 43 microseconds.
5. Connect to the module and watch for `TS_EVENT_READY`, then `TS_EVENT_IMEI`.
6. Send a message only after you have activated the modem and set `TS_TOPIC_ID`.
7. Before deployment, add a hardware watchdog kicked once per main-loop pass, and decide what your product does after `TS_FAULT`. See [Design notes](DESIGN.md#watchdog).

Run the host tests (`ctest`) with your compiler's warnings enabled to confirm the portable code builds cleanly. They do not test your port.

### 2026-11-03 engineering revision

The [engineering baseline](ENGINEERING.md) supersedes earlier examples where they
show a placeholder payload, imply that every FAULT is powered down, assume
unbounded clock/console waits, or imply a target build was not performed.
`TS_FAULT_HOLD` needs continued polling and reviewed hardware recovery; restart
also checks BOOTED. `ts_f169_init()` returns success/failure. Console is disabled
by default and best-effort when enabled. Follow the new order/build and hardware
validation procedures before adapting this bench example to a product.

## Rev A integration contract — 2026-11-15

- VIN is supplied by the application. `power_start`/`power_stop` control ENABLE only. `STOPPED` confirms ENABLE/BOOTED low; capacitor-backed rails can still be energized.
- `supply_ready` must represent qualified stored energy, not charger PG. `TS_CHARGING` keeps ENABLE low and waits without blocking. Charge timeout or pre-enable failure reports `TS_ERR_SUPPLY` without asserting ENABLE. Charge timeout defaults to 120 s; boot and shutdown each have independent 30 s defaults. These are software policies, not measured modem limits. Shutdown may cancel charging; once ENABLE rises, the complete boot/shutdown handshake remains required.
- The F169 bench adapter assumes VIN valid before `ts_f169_init()` and continuously applied thereafter. It waits `TS_F169_CHARGE_WAIT_MS` (60 s provisional), once per initialization. VIN interruption requires controlled recovery and supply requalification. A timer alone cannot detect brownout; production integration requires measured charge limits or a qualified supply monitor.
- `TS_EVENT_READY` means protocol initialization completed. `ts_bear_is_ready()` can remain true during an IMEI/signal query. Use `ts_bear_can_send()` and check `ts_bear_send() == TS_OK` before retiring a pending payload. Call mutating APIs in the main loop, not callbacks. No automatic retransmission follows an unknown delivery result.
- Poll every 1 ms for the reference configuration. At 230400 8N1 the input rate is 23040 bytes/s: a 64-byte poll budget requires a sustained interval below 2.78 ms. A 128-slot ring has 127 usable bytes, about 5.51 ms burst capacity. Budget both drain throughput and worst-case interrupt/loop latency; validate on the target.
- Unpowered host inputs must tolerate driven module outputs or have isolation. Buffer Ioff only applies when the buffer supply itself is zero. A disabled host UART does not establish electrical isolation.
- Adding `supply_ready` changes the platform struct and CHARGING changes state values. Rebuild all objects together; use symbolic enums and zero-initialize platform structs. NULL supply gates remain supported only under the explicit external-qualification contract.
