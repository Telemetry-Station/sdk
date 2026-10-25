# 05 Antenna and RF

Document ID: BEAR-RF-9704 | Revision: P1 | Status: Preliminary

## Overview

The BEAR Module 9704 operates in the Iridium L-band (1616 to 1626.5 MHz). Performance depends heavily on the antenna and on what it can see of the sky. The family is designed to offer two paths:

| Configuration | Description | Status |
|---|---|---|
| Connector | RF connector for an antenna you choose and qualify | In development |
| Qualified antenna | A BEAR-supplied Iridium antenna, characterized with the module | In development |

Final configuration names and part numbers are published with the ordering information at Rev A.

## RF specifications

| Parameter | Value |
|---|---|
| Frequency range | 1616 to 1626.5 MHz (Iridium L-band) |
| Polarization | Right-hand circular, as required by Iridium |
| Impedance | 50 ohm |
| Connector type | Coming soon |
| Maximum conducted output power | Coming soon |
| Receive sensitivity | Coming soon |
| Antenna gain and pattern (qualified antenna) | Coming soon |
| Required antenna gain range (your own antenna) | Coming soon |

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
| Qualified BEAR antenna | Characterized with the module | Availability published at Rev A |

## Link design

Message delivery time depends on satellite visibility. Plan for variable delivery time. Build your firmware around events, not fixed delays: wait for `TS_EVENT_MO_DELIVERED`, with a timeout that fits your application. Use the SDK's signal query (`ts_bear_request_signal`) to log signal quality during commissioning.

## Regulatory note

Antenna and installation choices affect RF exposure and regulatory compliance. When you integrate a module with your own antenna, guidance on the approved antennas and the labeling and documentation you need is published with the certification package. See [09 Compliance and quality](09_Compliance_and_Quality.md).
