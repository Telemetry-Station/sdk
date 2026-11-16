# BEAR Module 9704

The satellite connection for your remote sensor. BEAR combines an Iridium 9704 modem, power circuitry and an antenna connection. Your microcontroller reads the sensor and runs the matching [Telemetry Station SDK](../Telemetry_Station_SDK/README.md).

**[Start with one sample →](../Telemetry_Station_SDK/docs/GETTING_STARTED.md)** Try the software on your computer, then check the supported hardware path.

## Where the product stands

The Rev A board design is in review. It has not yet been built, physically validated or certified. Published electrical values are preliminary design values. The SDK is alpha, with computer tests and an MSP430F169 reference integration.

Target sales availability is **April 1, 2027**, subject to validation, approvals and production readiness. Kit contents and firm availability are not confirmed. [Check the release milestones](RD_CHECKLIST.md).

## What you supply

- A compatible microcontroller and its firmware. MSP430F169 is the reference implementation; other hosts need an implemented and validated adapter.
- A regulated power source, compatible connections and an antenna suitable for the engineering configuration.
- An activated service and a configured destination for live messages.

The intended kit experience is **connect → load the sample → activate → send once → see the reading**. A complete kit and tested board packages are still needed to make that experience independent of an engineer.

## Find the answer you need

| Your question | Read this | Outcome |
|---|---|---|
| Does this fit my product? | [Product brief](docs/01_Product_Brief.md) | What BEAR does and what you still supply |
| Can I buy or evaluate it? | [Ordering and support](docs/11_Ordering_and_Support.md) | Availability, planned contents and contact path |
| How does the whole system work? | [System architecture](docs/02_System_Architecture.md) | The path from sensor to destination |
| How do I connect it? | [Integration guide](docs/06_Integration_Guide.md) | Engineering setup and success checks |
| What are the pins and limits? | [Datasheet](datasheet/BEAR-9704_Datasheet.md) | Preliminary interface values and their status |
| What must my power supply do? | [Power management](docs/04_Power_Management.md) | Source sizing, charging and shutdown requirements |
| Where does the antenna go? | [Antenna guide](docs/05_Antenna_and_RF.md) | Placement and qualification constraints |
| Will it fit my enclosure? | [Mechanical and environmental](docs/08_Mechanical_and_Environmental.md) | Dimensions, mounting and unverified environmental targets |
| What does the firmware control? | [Firmware interface](docs/07_Firmware_Interface.md) | Signals, events and safe sequencing |
| What has been verified? | [Requirements and verification](docs/10_Requirements_and_Verification.md) | Required evidence, not just design intent |
| Is it approved for deployment? | [Compliance and quality](docs/09_Compliance_and_Quality.md) | Approval status and release work |

Detailed connection rules are in the [hardware interface](docs/03_Hardware_Interface.md). Public component and layout summaries are indexed under [hardware](hardware/README.md). Unfamiliar terms are in the [glossary](GLOSSARY.md).

## Reading specification status

**Defined** means a decision has been documented. **Design value** means calculated or selected, not measured. **Target** means intended performance. **Pending decision** means unresolved. None of these means hardware validation has passed.

Controlled documents identify their revision. P-numbers are preliminary document revisions; hardware uses letters. Public text is covered by the repository [MIT license](../LICENSE). Trademarks identify their respective owners.

**Next:** [check your setup path](../Telemetry_Station_SDK/docs/GETTING_STARTED.md#3-pick-your-path), or [contact us](https://telemetrystation.com/contact) with your application and host board before selecting hardware.
