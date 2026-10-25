# 06 Integration Guide

Document ID: BEAR-IG-9704 | Revision: P1 | Status: Preliminary

How to connect a BEAR Module 9704 to a microcontroller and bring it up with the Telemetry Station SDK. This guide uses the TI MSP430F169, the first test platform, as a worked example. Any microcontroller with one UART and two GPIO pins works the same way.

## What you need

| Item | Notes |
|---|---|
| BEAR Module 9704 | Evaluation units: coming soon |
| Power source | Battery or DC supply within the datasheet range (published at Rev A) |
| Iridium antenna | Included or connected, per configuration. Needs a clear sky view |
| Host microcontroller | One UART at 230400 baud, two GPIO pins, a millisecond timer |
| USB-serial adapter | Optional, for a debug console |
| Telemetry Station account | For the calculator and activation |

## Connections

| Host side | Module signal | Notes |
|---|---|---|
| GPIO output | ENABLE | Hold low until your supply is stable |
| GPIO input | BOOTED | Do not use the UART until this is high |
| UART TX | RXD | Cross-over: your TX goes to the module's RXD |
| UART RX | TXD | Your RX comes from the module's TXD |
| Ground | GND | Common ground is required |
| Supply | VIN | Within the datasheet range |

Keep UART wires short. Add series resistance or a level translator if your host logic voltage differs from the module's.

### Worked example: MSP430F169 test bench

These pins match the SDK reference port ([`ports/msp430f169`](../../Telemetry_Station_SDK/ports/msp430f169)). They are test-bench defaults you can override at compile time.

| MSP430F169 pin | Function | BEAR Module signal |
|---|---|---|
| P1.0 | `TS_F169_ENABLE` output | ENABLE |
| P1.1 | `TS_F169_BOOTED` input | BOOTED |
| P3.6 (UTXD1) | USART1 transmit | RXD |
| P3.7 (URXD1) | USART1 receive | TXD |
| P3.4 / P3.5 (USART0) | Optional debug console, 115200 baud | USB-serial adapter |
| GND | Ground | GND |

The F169 reference port uses an external 8 MHz crystal on XT2 for the main clock. The internal oscillator is not accurate enough for 230400 baud.

## Bring-up checklist

Work through these in order. Stop at the first step that fails.

1. **Bench supply first.** Power the module from a bench supply with a current limit, with ENABLE low and no antenna transmitting. Confirm that BOOTED is low.
2. **Antenna.** Connect the antenna. Do not enable the module without an antenna or a matched load.
3. **Build the SDK tests.** Run the host tests in `Telemetry_Station_SDK` to confirm your toolchain and settings.
4. **Add your platform file.** Implement the `ts_platform_t` functions for your microcontroller, or start from the F169 port. See [07 Firmware interface](07_Firmware_Interface.md).
5. **Boot.** Call `ts_bear_start()`. Confirm ENABLE goes high and BOOTED follows. The SDK reports `TS_EVENT_READY` after the serial handshake.
6. **Read the IMEI.** The SDK reports `TS_EVENT_IMEI`. Print it with `ts_activation_text()`. The 15-digit IMEI is also planned to appear on the module label.
7. **Activate.** Use the printed links: size your plan in the calculator, then activate with the IMEI. Set the topic ID you receive as `TS_TOPIC_ID`.
8. **First message.** Move outdoors with a clear view of the sky. Send one message and wait for `TS_EVENT_MO_DELIVERED`.
9. **Shutdown.** Call `ts_bear_shutdown()`. Wait for `TS_EVENT_STOPPED` (BOOTED low) before removing power. The [simulated demo](../../Telemetry_Station_SDK/examples/host/simulated_demo.c) shows the expected sequence.

## From prototype to product

| Step | Action |
|---|---|
| Payload | Generate your final `payload.c` and `payload.h`, and set `TS_MAX_PAYLOAD` to your bytes per message |
| Duty cycle | Start the module, send, confirm delivery, shut down. See [04 Power management](04_Power_Management.md) |
| Watchdog | Add your own watchdog policy. The SDK never blocks, so it can be serviced in the main loop |
| Duplicates | Keep a sequence number in your payload. The SDK never retries a send automatically |
| Production test | Read and record the IMEI of every unit. See [09 Compliance and quality](09_Compliance_and_Quality.md) |
| Enclosure | Test the antenna with the final enclosure closed |

## Troubleshooting

| Symptom | Likely cause | Action |
|---|---|---|
| BOOTED never goes high | ENABLE not reaching the module, supply too low, or energy storage still charging | Check ENABLE level and VIN. Allow the charge time. The SDK reports a timeout |
| Garbled serial data | Baud rate error | Use a crystal-based clock. Verify 230400 baud 8N1 |
| Boot works, no IMEI | UART TX and RX swapped | Cross the lines |
| Message stays accepted, not delivered | No satellite view, or the topic is not activated | Move outdoors. Check the activation and `TS_TOPIC_ID` |
| Reset during transmit | Supply sag | Check the source and wiring. See [04 Power management](04_Power_Management.md) |
| Start fails right after a shutdown | Module still shutting down | Wait for `TS_EVENT_STOPPED` before starting again |
