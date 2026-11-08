# Hardware Design Files

Customer-facing hardware summaries for the BEAR Module 9704. The current KiCad design is **Rev A** and its status
is **DESIGN REVIEW REQUIRED**. Before boards are ordered, the modem land pattern must be confirmed. Rev A is not
built, validated or certified. Native design files, fabrication outputs and internal engineering reports are not
included in this SDK; approved deliverables will be published separately with their applicable licenses.

The [R&D and release checklist](../RD_CHECKLIST.md) tracks hardware validation, certification, production readiness
and target availability.

| Folder | Contents | Status |
|---|---|---|
| [`interfaces/`](interfaces/bear-9704_signals.csv) | Machine-readable signal list with Rev A pin assignments and electrical limits | Rev A design values |
| [`bom/`](bom/BEAR-9704_BOM.csv) | Key-component summary by function; not a full assembly BOM | Rev A design values |
| [`schematics/`](schematics/README.md) | Block-level description; full schematic release pending | Customer summary |
| [`pcb/`](pcb/README.md) | Stack-up and layout summary; fabrication-file release pending | Customer summary |
| [`mechanical/`](mechanical/README.md) | Outline and mounting summary; customer drawings and model release pending | Rev A design values |
| [`manufacturing/`](manufacturing/README.md) | Build, test and serialization process | Process outline published |

Licensing and required third-party attribution for approved hardware design files will be stated with those files.
The repository MIT license does not grant rights to restricted vendor documents or separately licensed designs.
