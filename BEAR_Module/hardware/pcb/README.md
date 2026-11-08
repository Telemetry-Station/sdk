# PCB

Rev A layout summary for customer integration. Native KiCad files, fabrication outputs and internal RF calculations
are not included in this SDK. Approved design deliverables will be published separately.

| Topic | Rev A implementation |
|---|---|
| Board | 80 × 60 mm, R5 corners, 4 × M3 mounting holes |
| Stack-up | Four-layer FR-4, 1.6 mm, ENIG. L1 signal and RF; L2 solid ground; L3 power and ground; L4 signal and ground. 0.20 mm L1-L2 dielectric |
| RF section | Short 50 Ω controlled-impedance microstrip to the SMA, with solid ground beneath. Final geometry requires design review and RF verification |
| Power | Wide copper from the supercapacitor bank through the 10 mΩ sense resistor to the modem's V_BAT pads, with bulk and ceramic decoupling at the module |
| Digital | UART and handshake traces kept away from the RF path |
| Fabrication | IPC-6012 Class 2, IPC-A-600 Class 2, Tg ≥ 150 °C |
| Test access | TP1 bank, TP2 modem supply, TP3 3.3 V, TP4 CHG_STAT, TP5 CHG_PG, TP6 bank midpoint, TP7 GND, TP8 protected VIN, TP9-TP11 WAKE_OUT, WAKE_IN, SYNC. ENABLE, BOOTED and UART are probed at J1 |

Public design-release deliverables **[Pending decision]**: approved fabrication files, assembly drawings, schematic
and mechanical model with revision metadata, applicable licenses and required attribution. This summary is not a fabrication definition.
