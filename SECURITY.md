# Security

Please report vulnerabilities privately through GitHub's "Report a vulnerability" option on this repository rather than a public issue.

Supported versions: the latest release only. The project is alpha (see [CHANGELOG.md](CHANGELOG.md)).

Notes for users:

- The message CRC16 detects accidental corruption only. It does not authenticate or encrypt data.
- Treat all data received from the modem as untrusted input. The driver bounds every buffer and rejects malformed lines, and the tests run under AddressSanitizer. Fuzz testing is on the roadmap.
- The SDK adds no encryption or authentication. If your data is sensitive, protect it inside your payload.
- Do not commit topic IDs, account details, or other credentials you were given by Telemetry Station.
