# F169 / BEAR hardware validation

Status: **NOT YET RUN**. No board, probe connection, firmware programming, Iridium
transmission, GNSS reception or physical measurement was performed by these tools.

Record board serial/revision, BEAR revision, wiring drawing/hash, order/schema,
build manifest/image SHA, probe and CCS versions, calibrated instruments and raw
waveforms. Use the actual host schematic; the default bench mapping is P1.0 ENABLE,
P1.1 BOOTED, P3.6 USART1 TX to BEAR RX, P3.7 USART1 RX from BEAR TX. P3.4/P3.5
USART0 is the checkout console (115200 8N1, on in the bench order). XT2 is 8 MHz; Timer_A is the millisecond source.
Other GPIOs are unassigned. Pin assignments from other products do not apply here.

1. Inspect assembly, polarity, unpowered shorts and connector mating orientation.
2. With current-limited supply, verify rail voltage, ripple and sequencing against
   approved limits (currently unavailable for complete BEAR design).
3. Program only the reviewed image using CCS, verify flash read-back separately,
   and record it. Confirm reset/startup, clock/baud and disconnected-host behavior.
4. Capture OFF, BOOTING, READY and shutdown states. Measure actual I/O leakage,
   ENABLE/BOOTED order, UART timing and supply behavior; registers alone cannot.
5. Inject missing oscillator, delayed/stuck BOOTED, UART overflow, interrupted
   replies, resets and loss of supply. FAULT_HOLD must not be bypassed by resets
   during a sequencing test. Obtain the approved hardware recovery policy first.
6. Measure worst-case stack high-water and service latency with MT, MO and optional
   console/GNSS activity, then verify standalone operation without debugger halts.
7. Only on an activated authorized topic, compare actual optimizer bytes with the
   backend decoder; distinguish modem accepted, satellite delivered and backend
   accepted. Test unknown delivery without automatic duplicate submissions.
8. GNSS variant: RAM configuration/ACK, no-fix timeout, valid fix, stale FIFO,
   checksum faults, power isolation, antenna bias fault and Iridium TX desense.

For an existing modern CCS scripting session halted at a safe checkpoint:
```
const {capture} = require('/absolute/path/tests/hardware/ccs_snapshot.js');
const result = capture(session, {
  board_serial: '...', board_revision: '...', phase: 'OFF',
  image_sha256: '64 lowercase hex characters',
  pinmap_sha256: '64 lowercase hex characters'
});
```
Save the returned JSON through the station's evidence workflow. The helper never
connects, loads, halts or writes a target. Hashes are operator-supplied identities,
not automatic flash/schematic verification. CPU halts alter timing and watchdog
behavior; power-cycle/reset under the reviewed procedure before timing trials.

The bench image prints its own PASS/WARN/CHECK/SKIP checkout on the console each
cycle. Save that transcript with the evidence; it records what the firmware saw,
not the electrical measurements above.

Use PASS / IN REVIEW / NOT YET RUN / PENDING for each required observation, with numeric
limits, actual values, units and raw evidence. Do not collapse unmeasured items
into an overall PASS.
