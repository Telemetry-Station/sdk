# Getting started

Start by sending a simulated message on your computer. You do not need a module, antenna, account or paid service for that step.

**What is ready today?** The simulation is runnable. Connecting a real BEAR engineering unit still requires electronics and embedded programming skills; there is no validated beginner kit or universal one-click installer yet. Hardware setup time and satellite delivery time are not guaranteed.

| What you want to do | Start here |
|---|---|
| Understand the product without programming | Read the explanation below |
| See a message succeed on your computer | [Run the simulation](#2-see-it-work-with-no-hardware) |
| Connect hardware you already have | [Check support first](#3-pick-your-path) |
| Connect a sensor after the sample works | [Customize your project](#5-put-it-in-your-project) |

Keep the included example message for your first test. You can design a compact custom message later.

## 1. What problem this solves

Your sensor is somewhere with no cellular or Wi-Fi coverage. You want it to send a small reading, a few bytes every few minutes or hours, to a server you can query. The Iridium satellite network covers the whole Earth, so a satellite modem on your board can do that.

Talking to an Iridium modem takes real work: a serial command protocol, a careful power-up and power-down sequence, message segmenting, delivery tracking. This SDK does that part in portable C so your application only says "send these bytes" and gets "delivered" or "failed" back.

```text
your sensor --> your MCU + this SDK --> BEAR Module 9704 --> Iridium satellites --> cloud
```

The **module** is the satellite radio hardware. Your **microcontroller** is the small computer that reads your sensor. The **SDK** is software loaded onto that computer to operate the module. The module does not replace your microcontroller.

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
| **Firmware** | The program loaded onto your microcontroller; loading it is often called “flashing” |
| **Console** | A computer window showing messages printed by your device |
| **UART / serial** | The wired connection used to exchange data with the module |
| **MO / MT** | Mobile-originated (device to cloud) and mobile-terminated (cloud to device) |
| **Power management** | The module's supply circuitry. See below |

**Power management, briefly.** A satellite modem idles on little power but pulls large, short current bursts when it transmits. Many small supplies cannot provide those bursts and will brown out. The module smooths this out and supports the ENABLE/BOOTED sequence performed by the SDK and host. You only drive an `ENABLE` pin and watch a `BOOTED` pin.

## 2. See it work with no hardware

You need a downloaded copy of this repository, **CMake** (the build tool), and a **C compiler** (the tool that turns the code into a program). Open a terminal in the repository folder—the folder containing this README's parent `Telemetry_Station_SDK` directory. These commands are for macOS/Linux or a compatible shell with the tools already installed.

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

**Success:** both `TS_EVENT_MO_DELIVERED` and `TS_EVENT_STOPPED` appear. No real satellite message was sent, and the printed ID `300000000000001` is fictional. Do not use it for activation.

| What you see | What to do |
|---|---|
| `cmake: command not found` | Install CMake, reopen the terminal, and try again |
| C compiler not found | Install your operating system's C development tools, then rerun the configure command |
| The source directory does not contain `CMakeLists.txt` | Start in the downloaded repository folder, then run `cd Telemetry_Station_SDK` once |
| Build failed | Read the first error above the final failure message; report that error and your operating system |
| Program not found after a successful Windows build | With a multi-configuration generator, run `cmake --build build --config Debug`, then `build\Debug\simulated_demo.exe` in PowerShell |

That is the whole life of one message: start, ready, IMEI, send, delivered, shut down. The modem here is simulated ([`tests/sim_modem.c`](../tests/sim_modem.c)); the driver is the real one. Read [`examples/host/simulated_demo.c`](../examples/host/simulated_demo.c), about 80 lines, to see the API in use.

## 3. Pick your path

| You have | Do this |
|---|---|
| Nothing yet, just evaluating | Try step 2. For hardware availability, see [ordering and support](../../BEAR_Module/docs/11_Ordering_and_Support.md); no hardware purchase is needed for the demo |
| A TI MSP430F169 and a BEAR Module | Run the checkout in step 4, then step 5 with `ports/msp430f169`. Module availability: see the [roadmap](../../BEAR_Module/ROADMAP.md) |
| Arduino, ESP32, STM32 or another microcontroller | No ready-to-use BEAR adapter is included for these boards. An embedded developer must complete and validate [a port](PORTING.md). The separate Arduino LaunchPad example is for different hardware |
| A computer and a modem on a USB serial adapter | `ports/posix` supplies a UART transport only; real ENABLE/BOOTED control is required before use |

## 4. Check a BEAR Module on an MSP430F169 (flash and go)

**Engineering setup; not a beginner kit.** The F169 example checks the connection and prints a result for each step. With the topic left at zero, it reads the module ID and skips sending; no airtime activation is needed for that checkout. It does not establish electrical safety or production readiness.

Have an embedded engineer prepare these before proceeding:

| Required item | Why it is needed |
|---|---|
| BEAR Module 9704 engineering unit and its supplied setup procedure | The actual satellite radio and revision-specific power checks |
| MSP430F169 host board with an 8 MHz XT2 crystal | Runs the supplied reference firmware; a different MSP430 is not a drop-in replacement |
| Compatible programming probe and TI MSP430 compiler | Loads the firmware; a USB serial cable does not program this MCU |
| Regulated 5 V supply rated for at least 0.5 A, mating cable, and antenna | Powers and connects the module; confirm compatibility before applying power |
| USB-to-serial adapter with compatible logic levels and a terminal program | Shows the checkout results on your computer |

A complete validated shopping list, labeled cable kit and tested toolchain installation walkthrough are release requirements, not currently supplied products.

1. **Wire it.**

   | F169 pin | Connects to |
   |---|---|
   | P1.0 | J1 pin 5, BEAR `ENABLE` |
   | P1.1 | J1 pin 6, BEAR `BOOTED` |
   | P3.6 (USART1 TX) | J1 pin 8, BEAR `RXD` |
   | P3.7 (USART1 RX) | J1 pin 7, BEAR `TXD` |
   | P3.4 (USART0 TX) | USB serial adapter RX (console) |
   | GND | J1 pins 3 and 4, and adapter ground |

   Connect a regulated 5 V ±5%, ≥0.5 A source to J1 pins 1 and 2 and a suitable Iridium antenna to J2. J1 pin 9 is optional TX_ACTIVE; pin 10 is NC. Confirm connector orientation using the hardware drawing. You also need an 8 MHz XT2 crystal and compatible host inputs, including when the host is unpowered.

   Apply VIN with ENABLE low before MCU initialization. Qualify stored energy using the engineering unit’s supplied test procedure. The F169 port waits 60 s from initialization before ENABLE, a provisional bench policy requiring EVT validation; PG is not charge-ready. Keep VIN applied between cycles. See [power requirements](../../BEAR_Module/docs/04_Power_Management.md).
2. **Build and flash.** In CCS, import [`ccs/bear9704_sdk_msp430f169.projectspec`](../ccs/bear9704_sdk_msp430f169.projectspec), build, and flash. From the command line, `python3 tools/build_f169.py --order orders/bench.json` builds an image with the console enabled. It does not flash the board; program the generated `firmware.out` with your probe.
3. **Open the console** at **115200 8N1** on the adapter. Any terminal works; colour is ANSI.
4. **Start from a confirmed stopped modem using the engineering unit’s setup procedure.** Supply qualification starts first; the first ENABLE waits for the provisional 60 s gate. Subsequent cycles start after a 10-minute idle interval (`report_interval_ms`) following completion.

This excerpt comes from the host test of the F169 firmware, including the charge wait. It is not a hardware measurement; the IMEI is fictional:

```text
[    0.036] PASS   Module off at reset  BOOTED low
[   15.042] WAIT   Module boot          charging power reserve; keep power connected
[   30.042] WAIT   Module boot          charging power reserve; keep power connected
[   45.042] WAIT   Module boot          charging power reserve; keep power connected
[   60.042] WAIT   Module boot          waiting for BOOTED to rise (0 s)
[   61.501] PASS   Module boot          BOOTED high after 1502 ms
[   61.504] PASS   Serial link          modem replied in 3 ms
[   61.511] PASS   SIM                  internal SIM selected
[   61.519] PASS   Radio active         operational state active
[   61.522] PASS   IMEI                 300000000000001
[   61.527] PASS   Satellite signal     4/5 bars
[   61.534] SKIP   Message queued       TS_TOPIC_ID not set
[   61.541] SKIP   Delivery             nothing sent
[   62.233] PASS   Power down           BOOTED low after 706 ms; safe to cut power
------------------------------------------------------------
 RESULT   PASS   8 pass  0 warn  0 check  2 skip   62.2 s
```

These are simulated timings, not a promised setup or delivery time. The IMEI is printed with activation links. `SKIP` on the send and delivery rows is expected while the topic is zero; a checkout `PASS` with those rows skipped does not mean a message was delivered.

### Send the included sample before adding your sensor

1. Copy the **real module's IMEI** from its checkout. Do not copy the fictional number above.
2. Arrange service activation and obtain your **topic ID** using the printed activation instructions. The included F169 application sample is 4 bytes; the SDK adds a 2-byte integrity check to its transmitted payload. Confirm service sizing with your provider.
3. Configure the topic in the build path you actually use:

   | Build path | Setting to change |
   |---|---|
   | Python build script | Set `topic_id` in `orders/bench.json` to your assigned number, then run `python3 tools/build_f169.py --order orders/bench.json` from `Telemetry_Station_SDK`. This creates a new `firmware.out` in the output directory printed by the script; program that image with your probe |
   | Imported CCS project | Change the project's `TS_TOPIC_ID=0` compiler definition to your assigned topic, rebuild, and program the resulting image. Editing `orders/bench.json` does not change an already imported CCS project |

4. Keep the included payload code and buffer settings unchanged. With the antenna installed and a clear sky view, run the checkout under the engineering setup procedure.
5. Look for **`PASS Delivery`**. “Message queued” only means the modem accepted it. Confirm the sample also appears at the service destination configured for your account; satellite confirmation does not prove your own dashboard received it.
6. After delivery, confirm **`PASS Power down`** before removing the module's input supply. If shutdown reports `CHECK`, follow the engineering unit's recovery procedure.

The F169 sample sends a counter and a fixed example temperature of **21.5 °C**. It is not reading a real temperature sensor. Repeated cycles are enabled; the default waits 10 minutes between completed cycles and can use airtime once a topic is configured. Stop the evaluation after a clean shutdown if you only want one sample. Later cycles print a one-line summary.

**RESULT** is PASS when every step passed, WARN when something works but needs attention (for example 0 bars indoors), and CHECK when a step needs your attention before the module can be used. A CHECK step marks the steps that depend on it as SKIP, and the report distinguishes shutdown from FAULT_HOLD. Keep polling after faults; only STOPPED confirms ENABLE/BOOTED low.

| Row shows | What to check |
|---|---|
| `WAIT` with `charging power reserve; keep power connected` | Expected while the first-start supply gate is waiting. The F169 bench policy waits 60 s; do not reset it to skip this step |
| Nothing at all on the console | Console wiring (P3.4 to adapter RX, common ground) and baud. If still blank, XT2: the firmware stops with everything off when the crystal does not start |
| Module off at reset CHECK `BOOTED stuck high` | `BOOTED` wiring or a module still powered from an earlier run |
| Module boot CHECK `BOOTED never rose` | Module supply, `ENABLE` wiring. The driver then holds `ENABLE` for safety, so Power down also reports `ENABLE held`; follow the engineering unit’s recovery procedure after correcting the fault |
| Serial link CHECK `no reply; check TX/RX crossover and baud` | TX and RX swapped or unconnected |
| SIM / Radio active / IMEI CHECK | The modem answered with an error: note the detail and the IMEI, and report it |
| Satellite signal WARN `antenna needs open sky` | Normal indoors. Go outside with a clear view of the sky |
| Delivery CHECK `not confirmed in time; outcome unknown` | Sky view, or topic not activated. Queued is not delivered; do not resend automatically |
| Power down CHECK `BOOTED still high` / `BOOTED low, ENABLE held` | The module did not stop cleanly. Check `BOOTED` and follow the engineering unit’s recovery procedure |

You can see every outcome without hardware: `build/test_checkout transcript 1234` prints a full run, and `build/test_f169_firmware_0 console nomodule` (or `swapped`) runs the real firmware on an emulated F169 with a missing module or crossed wires.

## 5. Put it in your project

Do this after the included sample is delivered. This section is for the developer integrating your sensor and microcontroller.

1. **Design your message.** Open the [Payload Optimizer](https://telemetrystation.com/payload-optimizer), describe your readings, and download `payload.c` and `payload.h`. Note the **bytes per message**.
2. **Connect your encoder.** Edit [`examples/common/app_payload.c`](../examples/common/app_payload.c) to call the generated code. Set `TS_MAX_PAYLOAD` to your bytes per message.
3. **Write or choose a port.** Seven required callbacks plus the supply-ready gate in a `ts_platform_t` struct connect the SDK to your UART, a millisecond clock, and two GPIO pins.
4. **Wire the module.** `ENABLE` (output from your MCU), `BOOTED` (input), UART `TXD`/`RXD` crossed, common ground. Check logic voltages on both sides before powering. The F169 wiring is in the [integration guide](../../BEAR_Module/docs/06_Integration_Guide.md).
5. **Run it.** The firmware prints the **IMEI** and the next steps. You do not need a topic ID for this.
6. **Activate.** Put your bytes per message into the [calculator](https://telemetrystation.com/calculators/certus), then [activate](https://telemetrystation.com/activate/certus) the IMEI. You get a **topic ID**.
7. **Set `TS_TOPIC_ID`**, rebuild, go outdoors with a clear view of the sky, and send.
8. **Wait for `TS_EVENT_MO_DELIVERED`.** Queued is not delivered.

The main loop is short:

```c
ts_bear_poll(&bear);                      /* every 1 ms; validate target service latency */
if (have_data && ts_bear_can_send(&bear)) {
    ts_result_t result = ts_bear_send(&bear, TS_TOPIC_ID, bytes, length);
    if (result == TS_OK) have_data = false;
    /* Otherwise retain the payload and handle result. Await delivery events. */
}
```

For a battery product, start the module only when you have something to send and shut it down after delivery. [`examples/msp430f169/main.c`](../examples/msp430f169/main.c) shows the pattern as a small state machine.

## 6. When it does not work

| Symptom | Likely cause |
|---|---|
| `ts_bear_init` returns `TS_ERR_ARGUMENT` | A function is missing from your `ts_platform_t` |
| Stays in `TS_CHARGING` | Supply gate not satisfied; inspect VCAP qualification. F169 first boot waits 60 s provisionally; template fails closed until implemented |
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
