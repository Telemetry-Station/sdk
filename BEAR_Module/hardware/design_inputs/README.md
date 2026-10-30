# Initial CAD-neutral Design Inputs

This folder is an engineering starting point for the BEAR Module 9704. It is CAD-neutral and intentionally does not contain a component-level schematic, routed PCB, manufacturing data, or validated electrical design.

| File | Purpose |
|---|---|
| [`BEAR-9704_Initial_Design_Basis.md`](BEAR-9704_Initial_Design_Basis.md) | Functional topology, preliminary placement guidance, known interfaces, and open design inputs |
| [`BEAR-9704_Block_Connections.csv`](BEAR-9704_Block_Connections.csv) | Machine-readable conceptual connections between functional blocks |

## Status and use

- Treat **Defined** items as product-level intent only; they are not yet verified component specifications.
- **Candidate reference** items come from public third-party material and must be checked against controlled Iridium or module-vendor documentation.
- **Open** items must be resolved before schematic capture or PCB layout.
- Do not use these files to select components, order boards, or manufacture hardware.

The public [Iridium 9704 KiCad library](https://github.com/ndoo/iridium-9704-kicad-lib) describes a candidate 88-pad, 31.5 mm × 42.0 mm modem package. It is a community reference, not an Iridium-controlled package drawing or design guide, and is not adopted as a footprint in this package. The public [Ground Control RockBLOCK 9704 library](https://github.com/rock7/RockBLOCK-9704) is useful firmware reference material but does not establish the BEAR carrier-board electrical design.
