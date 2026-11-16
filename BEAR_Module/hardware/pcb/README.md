# Board layout summary

BEAR is a separate board that mounts inside your product. This page explains the preliminary Rev A layout for integration review. You are not expected to fabricate it.

**For enclosure planning:** [use the mechanical requirements](../../docs/08_Mechanical_and_Environmental.md) and request the approved customer drawings before committing dimensions.

| Topic | Rev A implementation |
|---|---|
| Board | 80 × 60 mm, R5 corners, 4 × M3 mounting holes |
| Stack-up | Four-layer FR-4, 1.6 mm, ENIG. L1 signal and RF; L2 solid ground; L3 power and ground; L4 signal and ground. 0.20 mm L1-L2 dielectric |
| RF section | Short 50 Ω controlled-impedance microstrip to the SMA, with solid ground beneath. Final geometry requires design review and RF verification |
| Power | Wide copper from the supercapacitor bank through the 10 mΩ sense resistor to the modem's V_BAT pads, with bulk and ceramic decoupling at the module |
| Digital | UART and handshake traces kept away from the RF path |
| Fabrication | IPC-6012 Class 2, IPC-A-600 Class 2, Tg ≥ 150 °C |
| Test access | TP1 bank, TP2 modem supply, TP3 3.3 V, TP4 CHG_STAT, TP5 CHG_PG, TP6 bank midpoint, TP7 GND, TP8 protected VIN, TP9-TP11 WAKE_OUT, WAKE_IN, SYNC. ENABLE, BOOTED and UART are probed at J1 |

The layout is still in design review; RF and power performance require physical validation. Test pads are engineering access points, not the normal customer setup interface.

**Next:** [review module mounting and antenna clearance](../../docs/08_Mechanical_and_Environmental.md) for your enclosure.
