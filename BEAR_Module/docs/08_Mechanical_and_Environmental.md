# 08 Mechanical and Environmental

Document ID: BEAR-ME-9704 | Revision: P2 | Status: Preliminary (Rev A design values)

## Mechanical

| Parameter | Value |
|---|---|
| Form factor | Board-level module with a keyed wire-to-board host connector and an SMA antenna connector |
| Board dimensions (L x W) | 80 × 60 mm, 5 mm corner radii, 1.6 mm thick |
| Height | Tallest parts are C1/C2 supercapacitors standing on the top side: 20 mm body, about 22 mm above the board. Measure on EVT units |
| Mass | Weigh EVT units and publish **[Pending decision]** |
| Mounting | 4 × M3 holes, Ø3.2 mm, centered 4 mm from each edge (72 × 52 mm pattern), with no copper around them |
| Host connector | J1 Molex Micro-Fit 3.0 43045-1012, 2 × 5, 3.0 mm pitch, vertical through-hole, keyed. See [03](03_Hardware_Interface.md) |
| RF connector | J2 Amphenol 132134 SMA jack, vertical, for the Maxtena M1621HCT-P-SMA or another Iridium-compliant antenna |
| PCB | Four-layer FR-4, 1.6 mm, ENIG, IPC-6012 Class 2. Inner layer 1 is a continuous ground plane; RF microstrip on fabricator-controlled impedance |
| Indicators | None in Rev A. Charger status (CHG_STAT) and power-good (CHG_PG) are on test pads TP4 and TP5. LEDs are considered for Rev B **[Pending decision]** |
| Labeling | Product name, part number, serial number, IMEI and a scannable code |

Mechanical deliverables published at release:

| File | Status |
|---|---|
| Board outline and assembly drawings | Approved customer drawings will be published separately **[Pending decision]** |
| 3D model (STEP) | Customer model release **[Pending decision]**; not included in this SDK |
| Host-side mating part | Molex 43025-1000 housing with 43030-0007 crimp terminals (cable connection; no host PCB footprint needed) |
| Keep-out areas for antenna and heat | Antenna: clear sky view; RF-exposure distances pending regulatory review. Heat: U2 charger runs warm during the first charge; thermal data at EVT |

See [`hardware/mechanical/`](../hardware/mechanical/README.md).

## Environmental

Temperature ranges below are proposed **Target** values pending confirmation against component limits. All require the tests in the [verification plan](10_Requirements_and_Verification.md) at DVT before release.

| Parameter | Value |
|---|---|
| Operating temperature | −40 °C to +85 °C proposed target; confirm component limits and verify at DVT **[Pending decision]** |
| Storage temperature | −40 °C to +70 °C proposed target; confirm component and packaging limits and verify at DVT **[Pending decision]** |
| Humidity | 5-95% RH, non-condensing proposed; test profile and condensation exposure **[Pending decision]** |
| Vibration and shock | Define IEC 60068 methods, severities and pass criteria for intended installations **[Pending decision]** |
| Altitude | 2,000 m proposed operating target; derating and test scope **[Pending decision]** |
| Ingress protection | Bare board: none. Your enclosure determines IP rating |
| Conformal coating | None in Rev A. Reconsider after DVT humidity testing; option and material **[Pending decision]** |

## Thermal

The modem and regulators dissipate most heat during transmit bursts. In Rev A the largest heat sources are the modem during bursts and the U2 linear charger while it charges an empty bank (up to 2.2 W demand, limited by its 125 °C thermal regulation). Thermal data, derating curves and airflow guidance will be measured at EVT and published with the datasheet **[Pending decision]**.

## Handling

| Topic | Guidance |
|---|---|
| ESD | Handle in an ESD-protected area, following ANSI/ESD S20.20 practice |
| Moisture sensitivity | Module handling and packaging controls will be published with approved customer documentation **[Pending decision]** |
| Soldering | Not applicable to the module. The host cable crimps per Molex 43030 application tooling |
| Storage | Original packaging, controlled temperature and humidity |

## Environmental testing (planned)

Tests are planned against applicable IEC 60068 methods (temperature, humidity, vibration, shock). Severity levels and acceptance criteria for fixed outdoor sensing, marine/polar and mobile use **[Pending decision]** will be defined before DVT and recorded in its report.
