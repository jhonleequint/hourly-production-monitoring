# Desktop application (case study)

The desktop application is the core of the system. It runs on a PC next to the line, collects events from the line devices and keeps the production indicators up to date. It works fully offline inside the plant network.

> The source code is private because the application runs in production. This document covers the architecture, the technical decisions and how the application evolved. The `examples/` folder holds small, self-contained snippets written to illustrate those decisions; they are not extracted from the application.

## Requirements (Mar 2026)

- No internet on the shop floor: everything has to run on a closed local network.
- Read inspection results from industrial vision cameras over TCP.
- Run unattended on line PCs and TV panels.
- Never lose counted production after a power cut.
