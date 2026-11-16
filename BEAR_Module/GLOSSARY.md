# Glossary

A quick translation of the words used in setup and engineering references. You do not need to learn the modem protocol to try the sample.

**[Return to setup →](../Telemetry_Station_SDK/docs/GETTING_STARTED.md)**

| Term | Meaning |
|---|---|
| BEAR Module | The Telemetry Station product family of satellite connectivity modules |
| BEAR Module 9704 | The first family member, built around the Iridium 9704 modem |
| BOOTED | Module output signal. High when the modem has finished starting up, low once it has shut down |
| ENABLE | Start/stop control signal. The host must follow the full BOOTED handshake before changing it again |
| Certus | Iridium's broadband service family. The 9704 runs on this network |
| EVT / DVT / PVT | Engineering, design and production validation builds |
| GNSS | Global navigation satellite system (GPS and others) |
| HAL | Hardware abstraction layer. In the SDK, the `ts_platform_t` interface you implement for your MCU |
| IMEI | The 15-digit identifier of the modem. Used to activate it with Telemetry Station |
| IMT | Iridium Messaging Transport. The message service of the 9704, up to 100,000 bytes per message |
| JSPR | The 9704's JSON-based serial command protocol, implemented by the SDK |
| L-band | The radio band Iridium uses to talk to handheld-class terminals and modems (1616 to 1626.5 MHz) |
| MO / MT | Mobile-originated (device to cloud) and mobile-terminated (cloud to device) messages |
| MCU | Microcontroller: the small computer in your device that reads sensors and runs the SDK |
| Payload Optimizer | Telemetry Station tool that designs a compact message and generates `payload.c` and `payload.h` |
| Power management | Circuitry that supplies, conditions, stores and sequences power for the modem. See [Power management](docs/04_Power_Management.md) |
| SDK | Software development kit: the library, examples and tools used by your device’s firmware to operate BEAR |
| Topic | The destination ID your messages are sent to. Telemetry Station issues it when you activate |
| TX burst | The short period of high current draw while the modem transmits |
| Firmware | Software loaded onto your microcontroller; loading it is often called flashing |
| Port / adapter | Code connecting the portable SDK to a specific microcontroller’s pins, serial connection and clock |
| UART | Wired serial data connection between your microcontroller and BEAR |
| GPIO | A microcontroller pin used to read or drive a control signal |
| Payload | The application bytes in a message, such as temperature and sequence number |
| Schema | The agreed field layout, units and scale used to encode and decode a payload |
| CRC | Integrity check for accidental data corruption; it does not authenticate or encrypt |
| Queued / delivered / received by application | Accepted by the modem / acknowledged by the network / found at your configured destination; three different checks |
| Fault hold | Software keeps ENABLE asserted because interrupting boot is unsafe; continue polling and use the reviewed recovery procedure |
| Qualification | Recorded tests showing that a particular configuration meets its release requirements |
| SLA | Service-level agreement: a published commitment defining service scope and performance; not yet established here |
