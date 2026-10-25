# 08 Mechanical and Environmental

Document ID: BEAR-ME-9704 | Revision: P1 | Status: Preliminary

## Mechanical

| Parameter | Value |
|---|---|
| Form factor | Board-level module with a header-style host interface |
| Board dimensions (L x W x H) | Coming soon |
| Mass | Coming soon |
| Mounting | Mounting holes. Pattern and hole diameter published with the outline |
| Host connector | Coming soon |
| RF connector or antenna | Coming soon, per configuration |
| PCB | Coming soon (layer count, finish, thickness) |
| Indicators | Power and ready indicators. Final arrangement at Rev A |
| Labeling | Product name, part number, serial number, IMEI and a scannable code |

Mechanical deliverables published at release:

| File | Status |
|---|---|
| Board outline drawing (PDF, DXF) | Coming soon |
| 3D model (STEP) | Coming soon |
| Recommended host footprint | Coming soon |
| Keep-out areas for antenna and heat | Coming soon |

See [`hardware/mechanical/`](../hardware/mechanical/README.md).

## Environmental

All ranges below are **Target** values. They are confirmed by the tests in the [verification plan](10_Requirements_and_Verification.md) and published at Rev A.

| Parameter | Value |
|---|---|
| Operating temperature | Coming soon |
| Storage temperature | Coming soon |
| Humidity | Coming soon (non-condensing for the bare board) |
| Vibration and shock | Coming soon |
| Altitude | Coming soon |
| Ingress protection | Bare board: none. Your enclosure determines IP rating |
| Conformal coating | Coming soon (option under evaluation) |

## Thermal

The modem and regulators dissipate most heat during transmit bursts. Thermal data, derating curves and the recommended airflow or heat-spreading practice are published with the datasheet.

## Handling

| Topic | Guidance |
|---|---|
| ESD | Handle in an ESD-protected area, following ANSI/ESD S20.20 practice |
| Moisture sensitivity | MSL rating published with the BOM |
| Soldering | Not applicable to the module. Header and connector soldering guidance is published with the connector |
| Storage | Original packaging, controlled temperature and humidity |

## Environmental testing (planned)

Tests are planned against IEC 60068 (temperature, humidity, vibration, shock) and the product's own requirements. Severity levels follow the target use: fixed outdoor sensing, marine and polar, mobile assets. The final profile is published with the DVT report.
