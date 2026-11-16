# Is Telemetry Station right for your product?

BEAR Module 9704 and the Telemetry Station SDK are being built to bring small sensor readings back from places without cellular or Wi-Fi service. Your device reads the sensor; BEAR sends the message through Iridium; your configured service destination receives it.

**[Try the sample →](../Telemetry_Station_SDK/docs/GETTING_STARTED.md#2-see-it-work-with-no-hardware)** See a simulated reading without buying hardware or activating service.

## What you can evaluate today

| Part | Available | Still needed |
|---|---|---|
| Software | Alpha C99 SDK, automated computer tests and an MSP430F169 reference integration | Physical validation on BEAR and tested packages for additional boards |
| Hardware | Preliminary electrical, power and mechanical documentation | Engineering builds, qualification, approvals and confirmed kit availability |
| Sensor to insight | Included sample encoder, local decoder and simulated send | Demonstrated live delivery into a documented customer destination |
| Service | Links to message sizing and activation | Published delivery/export options, retention, service-level commitments and fleet provisioning details |

**Best use now: evaluate and prepare a prototype.** The complete beginner kit is not available. April 1, 2027 is a tentative sales target, subject to the [release criteria](../BEAR_Module/RD_CHECKLIST.md).

## What would your team need to do?

| Task | Who can do it today | Completion evidence |
|---|---|---|
| Describe the use case | Product owner | Readings, report frequency, locations, power source and destination |
| Run the computer sample | Someone with Python, CMake and C tools installed | One simulated send and decoded 21.5 °C sample |
| Connect real hardware | Embedded engineer with the engineering unit's setup procedure | Verified wiring, qualified power, correct module ID and clean shutdown |
| Add your sensor | Firmware developer | Actual sensor values encoded with the agreed schema |
| Receive useful data | Service/application owner | Received message decoded with units and associated with the right device |
| Approve deployment | Product and engineering owners | Physical validation, market approvals, supply and service terms accepted |

Reading this guide requires no embedded expertise. The current hardware integration does. A tested kit, supported board package and guided destination setup are the work needed to remove that dependency for first use. See the [beginner acceptance criteria](../BEAR_Module/RD_CHECKLIST.md#beginner-setup-acceptance-criteria).

## What will it cost?

Budget separately for hardware, airtime/service and integration work. Hardware prices and lead times are not confirmed. Once you know message size and frequency, use the [service calculator](https://telemetrystation.com/calculators/certus) to explore service sizing and request current commercial terms. A quote must also identify the antenna, host, supply and programming equipment included.

No integration-time or delivery-time guarantee is established by the computer demo. Measure setup effort on the chosen hardware and latency in the intended installation.

## What are you committed to?

- **Software:** the SDK is MIT-licensed. You can inspect and adapt it.
- **Network:** BEAR 9704 uses Iridium. Network service must be activated before a real send.
- **Data service:** Telemetry Station is the documented path; the driver does not require a Telemetry Station account to run locally.
- **Data format:** the included sample codec is readable C. Keep its schema identity with your receiver configuration so the values decode correctly.

## What counts as success?

A sample is complete when the intended device reading appears at your chosen destination with the correct value, units and device identity, and the module finishes shutdown. “Queued” alone does not mean delivered. Satellite delivery alone does not establish application receipt.

The local demo demonstrates software behavior. It does not establish coverage at your site, battery life, production reliability or cloud integration.

## Before making a buying decision

Confirm these with [Telemetry Station](https://telemetrystation.com/contact):

1. Exact kit contents, supported host and tool versions, price and availability.
2. Your message frequency, power budget, antenna placement and deployment environment.
3. Activation, destination configuration, decoding, export and retention requirements.
4. Approvals for your markets and remaining end-product obligations; see [compliance status](../BEAR_Module/docs/09_Compliance_and_Quality.md).
5. Support, lifecycle, sales and service terms. No service-level commitment is established by these preliminary documents.
6. Security needs. The SDK's integrity check detects corruption; it does not encrypt data or authenticate senders. See [security](../SECURITY.md).

**Next:** [request an evaluation discussion](https://telemetrystation.com/contact) with your sensor, reporting interval, location, power source, host board and desired data destination. You do not need to choose buffer sizes or understand the modem protocol to start that discussion.
