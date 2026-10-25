# PCB

Layout guidance for the BEAR Module 9704. Gerbers and fabrication files are published at Rev A.

| Topic | Guidance |
|---|---|
| Stack-up | Coming soon. Continuous ground reference under RF and serial routing |
| RF section | Controlled-impedance antenna trace, short, with ground stitching and keep-out around the antenna feed |
| Power | Wide, short paths for transmit-burst current. Place energy storage and decoupling close to the modem |
| Digital | Keep UART and handshake traces away from the RF path |
| Fabrication | Built to IPC-6012. Finish and thickness coming soon |
| Test points | Supply, ENABLE, BOOTED, UART |

Deliverables at release: Gerber, drill, pick-and-place, assembly drawing.
