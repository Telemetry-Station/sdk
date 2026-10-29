# PCB

Layout guidance for the BEAR Module 9704. Gerbers and fabrication files are published at Rev A.

| Topic | Guidance |
|---|---|
| Stack-up | Continuous ground reference under RF and serial routing; layer count and controlled-impedance details **[Pending decision]** |
| RF section | Controlled-impedance antenna trace, short, with ground stitching and keep-out around the antenna feed |
| Power | Wide, short paths for transmit-burst current. Place energy storage and decoupling close to the modem |
| Digital | Keep UART and handshake traces away from the RF path |
| Fabrication | IPC-6012; four-layer FR-4, 1.6 mm and ENIG proposed **[Pending decision]** |
| Test points | Supply, ENABLE, BOOTED, UART |

Deliverables at release: Gerber, drill, pick-and-place, assembly drawing.
