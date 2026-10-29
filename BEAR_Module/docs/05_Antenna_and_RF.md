# 05 Antenna and RF

Document ID: BEAR-RF-9704 | Revision: P1 | Status: Preliminary

## Overview

The BEAR Module 9704 operates in the Iridium L-band (1616 to 1626.5 MHz). Performance depends heavily on the antenna and on what it can see of the sky. The family is designed to offer two paths:

| Configuration | Description | Status |
|---|---|---|
| Connector | RF connector for an antenna you choose and qualify | In development |
| Qualified antenna | A BEAR-supplied Iridium antenna, characterized with the module | In development |

Final configuration names and part numbers **[Pending decision]** will be published with the ordering information.

## RF specifications

| Parameter | Value |
|---|---|
| Frequency range | 1616 to 1626.5 MHz (Iridium L-band) |
| Polarization | Right-hand circular, as required by Iridium |
| Impedance | 50 Ω nominal **[Pending decision]** |
| Connector type | Select and qualify per product configuration **[Pending decision]** |
| Maximum conducted output power | Publish from controlled Iridium 9704 OEM specification and verify the assembled RF path **[Pending decision]** |
| Receive sensitivity | Publish from controlled Iridium 9704 OEM specification and applicable test configuration **[Pending decision]** |
| Antenna gain and pattern (qualified antenna) | Publish measured/certified data for each supported antenna SKU **[Pending decision]** |
| Required antenna gain range (your own antenna) | Publish only after Iridium approval and RF/regulatory review **[Pending decision]** |

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
| Qualified BEAR antenna | Characterized with the module | Candidate SKUs and qualification scope **[Pending decision]** |

## Link design

Message delivery time depends on satellite visibility. Plan for variable delivery time. Build your firmware around events, not fixed delays: wait for `TS_EVENT_MO_DELIVERED`, with a timeout that fits your application. Use the SDK's signal query (`ts_bear_request_signal`) to log signal quality during commissioning.

## Regulatory note

Antenna and installation choices affect RF exposure and regulatory compliance. Approved antenna configurations, installation guidance, labeling and documentation **[Pending decision]** will be published with the certification package. See [09 Compliance and quality](09_Compliance_and_Quality.md).
