# 04 Power Management

Document ID: BEAR-PM-9704 | Revision: P1 | Status: Preliminary

## What power management means

A satellite modem is a demanding electrical load. Most of the time it draws little. When it transmits, it draws short bursts of much higher current. A supply sized only for a typical microcontroller can sag during a burst. The modem or the microcontroller can then reset, and messages fail in ways that are hard to diagnose.

A power-managed module takes this problem off your design. In the BEAR Module, power management means four jobs:

| Job | What it does for you |
|---|---|
| **Protect** | Guards the input against reversed connections, over-voltage and transients from cables and batteries |
| **Regulate** | Converts your supply into the exact voltages the modem needs |
| **Store energy** | Holds energy locally so transmit bursts do not collapse your supply or require an oversized source |
| **Sequence** | Starts and stops the modem in the order it requires, with a single ENABLE and BOOTED handshake |

## Power architecture

```mermaid
flowchart LR
    VIN[VIN] --> PROT["Input protection"]
    PROT --> LIM["Current limiting<br/>and charge control"]
    LIM --> STORE["Energy storage"]
    STORE --> REG["Regulation"]
    REG --> MODEM["Iridium 9704<br/>rails"]
    REG --> LOGIC["Interface logic"]
    EN[ENABLE] --> SEQ["Sequencing<br/>and supervision"]
    SEQ --> REG
    MODEM -->|boot status| SEQ
    SEQ --> BOOTED[BOOTED]
```

| Block | Function | Implementation |
|---|---|---|
| Input protection | Reverse-polarity and transient protection | Coordinate with final VIN range and installation profile **[Pending decision]** |
| Current limiting and charge control | Limits inrush and source current | Set from source limits, inrush and transmit-burst profile **[Pending decision]** |
| Energy storage | Supplies transmit-burst current | Size against Iridium OEM transmit profile and measured source impedance **[Pending decision]** |
| Regulation | Produces modem and interface rails | Set rails, tolerances and ripple from controlled OEM requirements **[Pending decision]** |
| Sequencing and supervision | Orders the rails and applies the ENABLE and BOOTED handshake | Defined |

## What it means for your design

- **Source sizing.** On-module energy storage is intended to buffer transmit bursts. The required source capability and any reduction from peak modem current will be established by the power design and load testing **[Pending decision]**; size from released specifications, not this design intent.
- **Charge time.** If the selected energy-storage design needs a charge interval, the startup delay and operating conditions will be measured and specified **[Pending decision]**. Do not assume a charge time before it is published.
- **Brown-out ride-through (target).** Hold-up behavior and time will be set from modem shutdown requirements and verified under load **[Pending decision]**. Do not assume the modem can complete shutdown after input power is lost until this is specified.
- **Supply options.** A regulated DC input is the proposed baseline; permitted source types and the VIN range will be frozen after review of OEM requirements and the power design **[Pending decision]**.

## Power budget worksheet

Use this template to size your supply once the datasheet values are published.

| State | Module current | Duration | Per message |
|---|---|---|---|
| Off (ENABLE low) | Characterize at specified VIN and temperature **[Pending decision]** | Most of the time | |
| Boot | Characterize; publish peak and integrated charge **[Pending decision]** | t_BOOT | |
| Idle, registered | Characterize against modem state **[Pending decision]** | Until sent | |
| Transmit burst | Derive from OEM profile and verify peak/average **[Pending decision]** | Per message | |
| Shutdown | Characterize at specified VIN and temperature **[Pending decision]** | t_DOWN | |

Energy per message depends on how long the modem needs to find a satellite. Sky view and antenna quality matter. See [05 Antenna and RF](05_Antenna_and_RF.md).

**Duty cycling.** For battery systems, the lowest energy per message comes from keeping the module off between reports, starting it just before a send, and shutting it down after delivery is confirmed. The SDK supports this with `ts_bear_start()` and `ts_bear_shutdown()`.

## Operational recommendations

| Recommendation | Reason |
|---|---|
| Keep wiring between your supply and VIN short and heavy | Voltage drop during burst charging |
| Place the module near its source and keep its antenna cable short | Reduces losses and noise |
| Do not remove VIN while BOOTED is high | Allow a clean shutdown |
| Keep ENABLE low during your own power-up | Avoids starting the modem before your supply is stable |
| Use the SDK's `TS_EVENT_STOPPED` before cutting power | It confirms the shutdown handshake finished |
