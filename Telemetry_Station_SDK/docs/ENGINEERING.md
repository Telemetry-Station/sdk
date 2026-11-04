# Embedded engineering baseline

The SDK remains a portable C99 **host** library for BEAR, not firmware inside the
BEAR module. The F169 example is a bench integration, not an approved production
sensor application. Its sample temperature is synthetic; topic zero blocks sends.
An order with a real topic is for explicitly identified bench messages until the
application's sensor acquisition and payload semantics are implemented and tested.

## Practices

Frozen feature orders, explicit hardware mappings, no heap, bounded service work,
safe defaults, real target links, fault injection, coverage with honest scope,
debugger evidence, and independent production acceptance criteria.

## Build and order contract

`orders/bench.json` is the general-purpose starting order. JSON booleans select
MT, GNSS, console and watchdog; payload size, line capacity, topic and cadence are
validated. Unknown keys, numeric booleans, invalid topics and mismatched schema
identity fail. There is no implied customer production configuration.

```
python3 tools/build_f169.py --order orders/bench.json --ccs-root /path/to/ccs
python3 tests/run_matrix.py --target
python3 tests/coverage.py
```

Build output uses a fresh directory. Its frozen order, source/compiler/linker
hashes, image hash, map and manifest travel together. The manifest writes local paths as `<SDK>`,
`<CCS_ROOT>` and `<OUTPUT>` so it can be shared. TI MSP430 CGT compiles and
links for F169 with 512 bytes of reserved stack and no heap. The build then
runs `ofd430 --call_graph` on the image and computes the worst-case static stack:
the deepest main-loop path plus the nested interrupts, with every indirect call
assumed to reach any port or example function. It fails when less than 64 bytes
of the reserved stack remain and records `stack_static` in the manifest. The
bench order measures 384 of 512 bytes. This static bound excludes recursion
(none exists) and is not a high-water measurement. The CLI never programs a target. A failing build
cannot leave a new PASS manifest beside a stale image.

Import `ccs/bear9704_sdk_msp430f169.projectspec` in CCS for the default bench
configuration. GUI import/debug are NOT YET RUN. Custom CLI orders are not silently
selected in CCS: import/include the corresponding frozen configuration and
compare defines and image provenance before programming. Default spec and CLI
both reserve 512 bytes stack; the vendor linker file defines the real device.

## Power and scheduling changes

Start refuses BOOTED high even after a shutdown timeout. Unexpected BOOTED loss
isolates the UART and reports an error. If a failure occurs with BOOTED low,
`TS_FAULT_HOLD` retains ENABLE and rejects restart. Continue polling: a late
BOOTED rise permits ENABLE deassertion and waiting for BOOTED low. A permanently
stuck signal needs a reviewed hardware recovery procedure. This conservative
hold follows the documented power sequence and does not cut power on a guess. It may consume power indefinitely, so the
system must surface the fault. Do not reinitialize the driver to bypass it.

F169 safe GPIO/UART state precedes crystal startup. XT2 startup has a finite
iteration budget; `ts_f169_init()` now returns `bool`. On failure the example
stops with the interface inactive. The iteration budget is not a calibrated time.
The bench order and CCS project enable the checkout console (USART0, P3.4,
115200 8N1); `send_only` and `gnss_interface` leave it off and compile it out.
The console is a 128-byte ring drained by the USART0 TX interrupt. Writes are
all-or-nothing and never wait: the checkout keeps its pending text and retries on
a later loop, so nothing is lost and the modem UART is never starved. See
[Getting started](GETTING_STARTED.md#4-check-a-bear-module-on-an-msp430f169-flash-and-go)
for the checkout transcript and troubleshooting table. Watchdog requires a verified ACLK source and is off in the bench order.
The bench example explicitly consumes/discards MT; product applications must
replace that policy rather than interpret unauthenticated MT as commands.

Never equate accepted with delivered. A timed-out MO has unknown delivery status;
the driver does not automatically resend. The application/backend must implement
its duplicate policy. Service the UART frequently: the default 127 usable RX
bytes fill in roughly 5.51 ms at 230400 8N1. Validate worst-case service latency.

## Payload Optimizer contract

`examples/common/generated/` is an actual export from the Telemetry Station
Payload Optimizer, network key `certus`, schema `ts-payload-v1-5a82b543`:
unsigned 16-bit sequence followed by signed 16-bit temperature in tenths C.
The golden example is `1234ff85`. `app_payload.c` calls this generated codec;
it is no longer a handwritten placeholder. The optimizer adds no modem CRC;
the transport appends its own CRC. Store the schema ID in deployment/backend
metadata; it is not automatically sent in every message.

To change fields, export header/source/test from the optimizer, retain its schema
and example metadata, update the binding and order schema ID, then run host golden
vectors and the F169 link. `TS_MAX_PAYLOAD` must fit the export. Latitude/longitude
are not silently added by enabling GNSS; an explicit new schema is required.
`ts_bear_send()` remains available for arbitrary application bytes.

## Optional LEA-M8S-0-10

`ts_gnss.h` supplies a compile-time optional, bounded UBX NAV-PVT acquisition API.
The integrator owns a nonblocking byte source, reviewed power/isolation callback,
receiver RAM configuration and transport flushing at acquisition start. NAV-PVT
must be enabled for the actual receiver protocol. The API does not write flash
or assume receiver default output. It accepts checksum-valid 3D `gnssFixOK` fixes,
checks geographic bounds, and uses integer coordinates. No-fix data is not a
transport error; acquisition ends at its absolute budget. Zero budget never
powers on. Polling consumes at most 32 bytes per call, handles wraparound, and
powers down on completion/cancel/timeout/I/O failure. Use coordinates only when
status is `TS_GNSS_FIX`; inspect power-off failures separately.

No F169 GNSS power pin is assigned. USART1 belongs to BEAR; USART0 would conflict
with console. An integrator can use a dedicated UART/DDC adapter after hardware
review. The target matrix compiles the optional API; the bench app does not call
it and the linker can discard it. Therefore its RAM numbers do not include an
instantiated GNSS context or additional transport rings. Measure the integrated
variant before release. Source protocol: u-blox UBX-13003221 R28, NAV-PVT.

## Evidence and production

`tests/coverage.py` builds two configurations (GNSS and MT enabled, with the
driver's IMEI query at READY on and off), merges gcov counts per source and fails
below 100% line **and** branch coverage of `src/*.c`, the checkout
(`examples/common/bear_checkout.c`), the F169 port and console, and the F169
`main.c`. Branches that cannot be reached are listed with a reason in
`tests/branch_waivers.json`; a waiver that becomes reachable or stale fails the
gate. Raw reports and missed-line lists are retained. This is not MC/DC,
generated-code, ISR timing or hardware coverage.

The F169 `main.c` is tested unmodified: `test_f169_port.c` runs it against an
emulated F169 (GPIO, USART0/1, Timer_A, XT2, low-power sleep) wired to the
simulated modem, with variants for no topic, a topic, and the console compiled
out. Scenarios include a healthy module, missing module, swapped TX/RX and a
dead crystal. `test_checkout.c` drives the checkout through every step outcome
(PASS, WARN, CHECK, SKIP, pending/WAIT), missing, late, corrupt and error replies,
BOOTED stuck high or low, brownouts, a full console and an application sharing
the driver.

`tests/run_matrix.py` builds and tests every MT x GNSS x IMEI-on-ready host
combination with sanitizers and warnings as errors; `--target` also builds each
MT/GNSS order and a headless image with TI cl430 under the RAM, flash and stack
gates. Hardware stubs
are only syntax/register models. Never include `tests/stubs` in a target build.

`tools/production_record.py` takes an independently approved policy and a unit
record. Missing tests, limits, image verification, evidence hashes or final state
produce HOLD. Firmware feature flags cannot remove required policy tests.
RECORD_ACCEPTED means record consistency only, never permission to ship.
Separate release qualification, station qualification, per-unit tests and sampled
characterization. No actual product policy or limits are approved here.
