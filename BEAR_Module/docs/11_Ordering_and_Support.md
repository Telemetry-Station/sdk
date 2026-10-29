# 11 Ordering and Support

Document ID: BEAR-OS-9704 | Revision: P1 | Status: Preliminary

## Part numbers

| Part number   | Description                                               | Availability                                           |
| ------------- | --------------------------------------------------------- | ------------------------------------------------------ |
| BEAR-9704-EVK | Evaluation kit                                            | Planned; firm availability date **[Pending decision]** |
| BEAR-9704-xxx | Production configurations (antenna and connector options) | Configuration suffixes **[Pending decision]**          |

The `xxx` suffix will identify the antenna and interface configuration. Part-number assignments **[Pending decision]** will be published with the datasheet.

## Ordering path

| Stage      | Description                                 |
| ---------- | ------------------------------------------- |
| Evaluation | Single units for bench testing with the SDK |
| Pilot      | Small quantities for field trials           |
| Production | Volume orders with lot traceability         |

Pricing and lead times **[Pending decision]** will be published before ordering opens. Firm evaluation-kit dates will be published after engineering validation, supply readiness and fulfillment planning are confirmed.

## Packaging and labeling

Each module ships in ESD-protective packaging. The label carries the part number, serial number, hardware revision, and IMEI as text and as a scannable code (target).

## Telemetry Station service

Module hardware and airtime are separate. Activate the IMEI at telemetrystation.com after sizing your messages with the Payload Optimizer and calculator. See the [root README](../../README.md).

## Lifecycle

| Policy                      | Status                                                                        |
| --------------------------- | ----------------------------------------------------------------------------- |
| Revision control            | Letter revisions with a change history in the datasheet                       |
| Product change notification | Advance notice for form, fit or function changes. Period published at release |
| Long-term availability      | Commitment period and lifecycle policy **[Pending decision]**                 |

## Support

| Resource                   | Where                                                                        |
| -------------------------- | ---------------------------------------------------------------------------- |
| Firmware and SDK issues    | GitHub issues on this repository                                             |
| Security reports           | [Security policy](../../SECURITY.md)                                         |
| Hardware and sales         | [telemetrystation.com/contact](https://telemetrystation.com/contact)         |
| Returns and warranty (RMA) | Process and warranty terms **[Pending decision]**; publish before sales open |

## Service status and SLA

A public service status page and a service-level agreement are launch-readiness deliverables. Status-page URL, covered services, availability objectives, measurement window, exclusions, incident communication and support response targets **[Pending decision]** will be published before general availability. No service-level commitment is made by this preliminary document.

## Customer launch package

The public launch package is planned to include:

- Revision-A datasheet PDF and controlled hardware design files.
- Arduino, STM32 and Raspberry Pi SDK quickstarts, with tested platform/toolchain versions stated in each guide.
- Sample payload decoders and matching test vectors.
- Public product/service status page and approved SLA.
- Repository changelog (already available at [`../../CHANGELOG.md`](../../CHANGELOG.md)).
- Firm evaluation-kit availability dates, published once build, validation and supply are confirmed.

Publication timing and the exact scope of each item **[Pending decision]** are tracked in the [roadmap](../ROADMAP.md).
