# Glossary

| Term | Meaning |
|---|---|
| BEAR Module | The Pacific Gyre product family of satellite connectivity modules |
| BEAR Module 9704 | The first family member, built around the Iridium 9704 modem |
| BOOTED | Module output signal. High when the modem has finished starting up, low once it has shut down |
| ENABLE | Module input signal. Switches the modem on (high) or off (low) |
| Certus | Iridium's broadband service family. The 9704 runs on this network |
| EVT / DVT / PVT | Engineering, design and production validation builds |
| GNSS | Global navigation satellite system (GPS and others) |
| HAL | Hardware abstraction layer. In the SDK, the `ts_platform_t` interface you implement for your MCU |
| IMEI | The 15-digit identifier of the modem. Used to activate it with Telemetry Station |
| IMT | Iridium Messaging Transport. The message service of the 9704, up to 100,000 bytes per message |
| JSPR | The 9704's JSON-based serial command protocol, implemented by the SDK |
| L-band | The radio band Iridium uses to talk to handheld-class terminals and modems (1616 to 1626.5 MHz) |
| MO / MT | Mobile-originated (device to cloud) and mobile-terminated (cloud to device) messages |
| MCU | Microcontroller unit |
| Payload Optimizer | Telemetry Station tool that designs a compact message and generates `payload.c` and `payload.h` |
| Power management | Circuitry that supplies, conditions, stores and sequences power for the modem. See [Power management](docs/04_Power_Management.md) |
| SDK | The Telemetry Station SDK firmware in this repository |
| Topic | The destination ID your messages are sent to. Telemetry Station issues it when you activate |
| TX burst | The short period of high current draw while the modem transmits |
