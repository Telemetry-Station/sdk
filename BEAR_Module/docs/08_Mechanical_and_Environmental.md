# 08 Mechanical and Environmental

Document ID: BEAR-ME-9704 | Revision: P1 | Status: Preliminary

## Mechanical

| Parameter | Value |
|---|---|
| Form factor | Board-level module with a header-style host interface |
| Board dimensions (L x W x H) | Establish from enclosure and layout constraints; publish controlled drawing **[Pending decision]** |
| Mass | Measure production configuration and publish **[Pending decision]** |
| Mounting | Mounting holes proposed; pattern and hole diameter **[Pending decision]** |
| Host connector | Keyed, polarized connector proposed; family and pitch **[Pending decision]** |
| RF connector or antenna | Select and qualify per configuration **[Pending decision]** |
| PCB | Four-layer FR-4, 1.6 mm, ENIG proposed as a common fabrication baseline; stack-up and finish **[Pending decision]** |
| Indicators | Power and ready indicators proposed; final arrangement and meaning **[Pending decision]** |
| Labeling | Product name, part number, serial number, IMEI and a scannable code |

Mechanical deliverables published at release:

| File | Status |
|---|---|
| Board outline drawing (PDF, DXF) | Release controlled drawing with Rev A **[Pending decision]** |
| 3D model (STEP) | Release model aligned to Rev A mechanical design **[Pending decision]** |
| Recommended host footprint | Publish with selected connector **[Pending decision]** |
| Keep-out areas for antenna and heat | Define by RF/thermal design and verify with enclosure **[Pending decision]** |

See [`hardware/mechanical/`](../hardware/mechanical/README.md).

## Environmental

All ranges below are proposed **Target** values marked **[Pending decision]**. They require confirmation against component/OEM limits and the tests in the [verification plan](10_Requirements_and_Verification.md) before publication as Rev A specifications.

| Parameter | Value |
|---|---|
| Operating temperature | -40 °C to +85 °C proposed industrial target; component/OEM limits and test results **[Pending decision]** |
| Storage temperature | -40 °C to +85 °C proposed target; confirm against components and packaging **[Pending decision]** |
| Humidity | 5-95% RH, non-condensing proposed; test profile and condensation exposure **[Pending decision]** |
| Vibration and shock | Define IEC 60068 methods, severities and pass criteria for intended installations **[Pending decision]** |
| Altitude | 2,000 m proposed operating target; derating and test scope **[Pending decision]** |
| Ingress protection | Bare board: none. Your enclosure determines IP rating |
| Conformal coating | Not included in baseline unless environmental testing requires it; option and material **[Pending decision]** |

## Thermal

The modem and regulators dissipate most heat during transmit bursts. Thermal data, derating curves and recommended airflow or heat-spreading practice will be established by measurement and published with the datasheet **[Pending decision]**.

## Handling

| Topic | Guidance |
|---|---|
| ESD | Handle in an ESD-protected area, following ANSI/ESD S20.20 practice |
| Moisture sensitivity | Supplier MSL ratings and handling controls published with the BOM **[Pending decision]** |
| Soldering | Not applicable to the module. Header and connector soldering guidance is published with the connector |
| Storage | Original packaging, controlled temperature and humidity |

## Environmental testing (planned)

Tests are planned against applicable IEC 60068 methods (temperature, humidity, vibration, shock). Severity levels and acceptance criteria for fixed outdoor sensing, marine/polar and mobile use **[Pending decision]** will be defined before DVT and recorded in its report.
