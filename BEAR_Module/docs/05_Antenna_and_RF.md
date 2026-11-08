# 05 Antenna and RF

Document ID: BEAR-RF-9704 | Revision: P2 | Status: Preliminary (Rev A design values; not RF-tested)

## Overview

The BEAR Module 9704 operates in the Iridium L-band (1616 to 1626.5 MHz). Performance depends heavily on the antenna and on what it can see of the sky. The family is designed to offer two paths:

| Configuration | Description | Status |
|---|---|---|
| Connector | SMA jack (J2, Amphenol 132134) for an antenna approved for the released configuration | Rev A design complete; RF test at EVT |
| Reference antenna | Maxtena M1621HCT-P-SMA (100-00003-02), passive Iridium helical, 2.8 dBic, screws directly onto J2 | Selected for Rev A; characterization with the module at DVT |

Configuration part-number suffixes **[Pending decision]** will be published with the ordering information. The reference antenna is a design selection, not a qualified configuration.

## RF specifications

| Parameter | Value |
|---|---|
| Frequency range | 1616 to 1626.5 MHz (Iridium L-band) |
| Polarization | Right-hand circular, as required by Iridium |
| Impedance | 50 Ω. Rev A: 0.34 mm microstrip on a fabricator-controlled-impedance stack-up, about 8 mm long |
| Connector type | SMA jack, vertical |
| Conducted output power (modem) | Publish permitted specifications and verify the assembled RF path at EVT **[Pending decision]** |
| Receive sensitivity | Publish permitted specifications and verify the applicable test configuration **[Pending decision]** |
| Reference antenna gain | 2.8 dBic peak (Maxtena data). About 2.7 dBi after the 0.14 dB board trace loss |
| Required antenna gain and separation (your own antenna) | Publish approved antenna limits and RF-exposure guidance after regulatory review **[Pending decision]**. Do not treat the reference antenna as an approved installation |

## Choosing and placing an antenna

These rules apply to any Iridium installation.

1. **Clear sky view.** The antenna must see as much sky as possible. Trees, buildings, enclosures, ice, water and your own structure block the signal.
2. **Orientation.** Mount the antenna as its manufacturer specifies, usually vertical with its ground plane level.
3. **Ground plane.** Use the ground plane size the antenna requires. A patch antenna on too small a plane loses gain.
4. **Cable.** Use low-loss 50 ohm coaxial cable and keep it short. Every decibel of loss reduces your link margin.
5. **Separation.** Keep the antenna away from other transmitters, switching regulators, and sources of noise, including GNSS and cellular radios. Allow separation as given in the antenna's data.
6. **Enclosure.** Use RF-transparent materials in front of the antenna. Test with the enclosure closed.
7. **Test outdoors.** Validate in the real installation, not only on a bench near a window.

## Antenna options and what to consider

| Type | Strengths | Considerations |
|---|---|---|
| Patch (flat) | Low profile, good for fixed installs | Needs ground plane; sky view overhead |
| Helical or whip | Wide coverage, tolerant of orientation | Larger |
| Reference antenna (Maxtena M1621HCT-P-SMA) | Selected for Rev A; 2.8 dBic | Characterization with the module planned at DVT; additional SKUs **[Pending decision]** |

## Link design

Message delivery time depends on satellite visibility. Plan for variable delivery time. Build your firmware around events, not fixed delays: wait for `TS_EVENT_MO_DELIVERED`, with a timeout that fits your application. Use the SDK's signal query (`ts_bear_request_signal`) to log signal quality during commissioning.

## Regulatory note

Antenna and installation choices affect RF exposure and regulatory compliance. Approved configurations, antenna gain limits, separation distances, labeling and installation guidance **[Pending decision]** will be published with the certification package. BEAR Rev A is not certified; do not deploy it as a qualified radio product. See [09 Compliance and quality](09_Compliance_and_Quality.md).
