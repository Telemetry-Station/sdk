# Protecting your device and data

The SDK checks messages for accidental corruption. It does **not** encrypt readings or verify who sent an incoming message. A successful delivery or CRC check is not proof of confidentiality or authenticity.

**Found a vulnerability? [Report it privately →](https://github.com/Telemetry-Station/sdk/security/advisories/new)** Use GitHub’s “Report a vulnerability” option; do not post exploit details or credentials in a public issue. If that option is unavailable, [contact Telemetry Station](https://telemetrystation.com/contact) to arrange a private reporting channel before sending sensitive details.

## Before connecting a product

- Define how your application authenticates and protects sensitive payloads. Do not treat unauthenticated incoming data as device commands.
- Keep credentials, account details and assigned customer topic/device identifiers out of public commits, screenshots and issue logs. Use placeholders or synthetic values in examples.
- Review malformed-input and duplicate-message behavior. The driver bounds its buffers and tests run with memory-error detection; fuzz testing and independent review remain planned.
- Pin the SDK version used by your product and review changes before upgrading.

Supported versions: the latest release only. This is an alpha project; see the [changelog](CHANGELOG.md).

**Next:** include your security requirements in the [integration and deployment review](Telemetry_Station_SDK/tests/hardware/README.md). For ordinary setup errors, use the [troubleshooting guide](Telemetry_Station_SDK/docs/GETTING_STARTED.md#6-when-it-does-not-work).
