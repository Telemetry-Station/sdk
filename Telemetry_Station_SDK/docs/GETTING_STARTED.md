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

Your sensor is beyond cellular or Wi-Fi service. You want a small reading to arrive at a destination you can use. BEAR provides the Iridium connection; actual delivery still depends on service activation, antenna installation and sky view.

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

**Power management, briefly.** A satellite modem idles on little power but pulls large, short current bursts when it transmits. Many small supplies cannot provide those bursts and will brown out. The module smooths this out and supports the ENABLE/BOOTED sequence performed by the SDK and host. Your platform adapter qualifies the stored energy before startup; the SDK then sequences the `ENABLE` and `BOOTED` signals.

## 2. See it work with no hardware

**Goal:** send one simulated reading and see **21.5 °C** decoded on your computer. The value is example data, not a live sensor measurement.

You need a downloaded copy of this repository, Python 3, CMake and C development tools. Open a terminal in the repository folder (`sdk`). Check your tools:

```sh
python3 --version
cmake --version
```

Run the guided sample:

```sh
python3 Telemetry_Station_SDK/tools/try_sample.py
```

On Windows, use `py -3` instead of `python3` if that is how Python is installed. The script checks for CMake, builds in a temporary directory, runs the sample and removes the temporary build. CMake reports a missing C compiler during preparation.

Look for these lines among the diagnostic events:

```text
Sample sensor: 21.5 C (example data, not a live sensor). Sending once.
Sample payload (without transport CRC): 000000d7
Simulated delivery confirmed.
Module stopped. The simulated power-down is complete.
Local decode preview: sample 0, temperature 21.5 C
This preview decodes the local sample; it is not a cloud receipt.
```

**Success:** the command exits successfully with that decoded value and completed shutdown. The driver runs against a simulated modem. No hardware, satellite, account or cloud destination is involved. The printed module ID is fictional; do not activate it.

### Prefer manual build commands?

From the repository folder:

```sh
cd Telemetry_Station_SDK
cmake -S . -B build
cmake --build build --config Debug
./build/simulated_demo
```

For a Windows multi-configuration build, run `build\Debug\simulated_demo.exe` in PowerShell. Source: [complete demo](../examples/host/simulated_demo.c).

| What you see | Next action |
|---|---|
| Python is not found | Use `py -3` on Windows if installed, install Python 3, or use the manual CMake commands |
| CMake is not found | Install CMake, reopen the terminal and rerun the command |
| C compiler is not found | Install your operating system's C development tools, then rerun preparation |
| File or source directory not found | Open the terminal in the downloaded `sdk` folder; avoid running `cd Telemetry_Station_SDK` twice |
| Build error | Save the first compiler error and your operating system/tool versions; [report the issue](https://github.com/Telemetry-Station/sdk/issues) |
| You see delivery but no temperature or shutdown | The run is incomplete. Save the output and report the issue before proceeding |

**Next:** [check your hardware path](#3-pick-your-path). Do not buy a board based only on the demo working.

## 3. Pick your path

| You have | Do this |
|---|---|
| Nothing yet, just evaluating | Try step 2. For hardware availability, see [ordering and support](../../BEAR_Module/docs/11_Ordering_and_Support.md); no hardware purchase is needed for the demo |
| A TI MSP430F169 and a BEAR Module | Run the checkout in step 4, then step 5 with `ports/msp430f169`. Module availability: see the [roadmap](../../BEAR_Module/ROADMAP.md) |
| Arduino, ESP32, STM32 or another microcontroller | No ready-to-use BEAR adapter is included for these boards. An embedded developer must complete and validate [a port](PORTING.md). The separate Arduino LaunchPad example is for different hardware |
| A computer and a modem on a USB serial adapter | `ports/posix` supplies a UART transport only; real ENABLE/BOOTED control is required before use |

## 4. Check a BEAR Module on an MSP430F169

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
4. **Start from a confirmed stopped modem using the engineering unit’s setup procedure.** Supply qualification starts first; the first ENABLE waits for the provisional 60 s gate. The default bench configuration runs one checkout and then stays idle. Repetition is an explicit configuration choice.

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
3. Configure the topic in the build path you actually use. **With a valid topic, the next startup attempts one send automatically.** Prepare the antenna, sky view and console before starting the new image:

   | Build path | Setting to change |
   |---|---|
   | Python build script | Copy `orders/bench.json` to a local, uncommitted order file, set `topic_id` to your assigned number, keep `report_interval_ms` at `0`, then run `python3 tools/build_f169.py --order /path/to/your-order.json` from `Telemetry_Station_SDK`. This creates a new `firmware.out` in the output directory printed by the script; program that image with your probe |
   | Imported CCS project | Change the project's `TS_TOPIC_ID=0` compiler definition to your assigned topic, rebuild, and program the resulting image. Editing `orders/bench.json` does not change an already imported CCS project |

4. Keep the included payload code and buffer settings unchanged. With the antenna installed and a clear sky view, run the checkout under the engineering setup procedure.
5. Look for **`PASS Delivery`**. “Message queued” only means the modem accepted it. Confirm the sample also appears at the service destination configured for your account; satellite confirmation does not prove your own dashboard received it.
6. After delivery, confirm **`PASS Power down`** before removing the module's input supply. If shutdown reports `CHECK`, follow the engineering unit's recovery procedure.

The F169 sample sends a counter and a fixed example temperature of **21.5 °C**. It is not reading a real temperature sensor. The default runs **one checkout per startup**, then reports `Checkout complete; automatic repeats disabled.` Topic zero sends nothing. A valid topic enables one sample; startup is the deliberate send action. Start another run only after confirmed clean shutdown using the engineering unit’s recovery/setup procedure. Do not reset to bypass a fault. To opt into repeated reporting, set `report_interval_ms` in a Python build order, or `REPORT_INTERVAL_MS` in CCS, to a positive idle interval such as `600000` for 10 minutes after completion. Repeated sends can use airtime.

### See the decoded reading

A delivered message is useful only when you can find and interpret it. Confirm the destination and access method with your service provider before the live test; this repository does not provision a dashboard, webhook or inbox.

1. At the agreed destination, locate the message for your **actual module**, assigned topic and test time.
2. Confirm that its format uses sample schema `ts-payload-v1-5a82b543`. The schema ID is configuration metadata; it is not included in every message.
3. Obtain the message bytes as hexadecimal. From the repository root, run the included decoder with the **six bytes including transport CRC**:

   ```sh
   python3 Telemetry_Station_SDK/tools/decode_sample.py YOUR_RECEIVED_HEX
   ```

4. If the service already removed the two CRC bytes, use `--payload-only` with the remaining four bytes. This explicit mode cannot verify transport integrity. Try a local example now:

   ```sh
   python3 Telemetry_Station_SDK/tools/decode_sample.py --payload-only 000000d7
   ```

   Expected fields: `sequence: 0`, `temperature_c: 21.5`, and `integrity: "not_checked"`.

5. For a real first F169 sample, expect sequence `0` and `21.5 °C`. This temperature is synthetic. Match your source reading and destination record again after connecting a real sensor.

The decoder only supports the included schema. A checksum failure is a failed check, not a reading to display. A new payload schema needs its matching decoder. CRC verification detects accidental corruption, not the sender's identity.

**Next:** after the real sample appears at the destination and shutdown passes, [connect your sensor](#5-put-it-in-your-project).

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

1. **Keep the working host and connection.** For a different MCU, complete [the porting checklist](PORTING.md) and validate that hardware before proceeding.
2. **Read your sensor.** Replace the synthetic `reading = {0, 215}` in the [F169 application](../examples/msp430f169/main.c) with a validated sensor reading in tenths of a degree Celsius. Decide how missing or invalid readings are represented; do not transmit them as real measurements.
3. **Keep the sample format if it fits.** It carries a 16-bit sequence number and a signed temperature in tenths °C. If you need different fields, use the [Payload Optimizer](https://telemetrystation.com/payload-optimizer), retain the exported schema, and update the [sensor binding](../examples/common/app_payload.c), its header and receiver decoder together. Check buffer, RAM and stack capacity.
4. **Send once on your activated topic.** Use `ts_bear_can_send()` and check the submission result. Wait for delivery; a timeout can mean the outcome is unknown.
5. **Compare the actual reading with the decoded destination value.** Check units, scale, sequence number and device identity. Then confirm clean shutdown.
6. **Choose a reporting schedule.** Enable repeated cycles only after the single-message path works and you have reviewed airtime, battery use and duplicate handling.

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

**How big is it?** About 0.8 KB of RAM with defaults in the host build. A previous F169 build used about 18.9 KB of flash and 2025 of 2048 bytes of RAM including the 512-byte stack, with a 384-byte static worst-case stack estimate. These are baseline figures, not a measurement of the revised image. Rebuild with the TI toolchain and check the current manifest before programming. See [Design notes](DESIGN.md#memory) and the [engineering baseline](ENGINEERING.md).

**Will it retry a failed send?** No, on purpose. A retry after an unknown result can double-send. You decide.

**Is it secure?** The SDK checks integrity with a CRC. It does not encrypt or authenticate. Encrypt inside your payload if you need to.

**Where do I ask questions?** Open an issue on GitHub.

**Next:** use the [hardware validation procedure](../tests/hardware/README.md) before deployment. It checks the electrical behavior, fault recovery and timing that a computer simulation cannot establish.
