# Desktop application (case study)

The desktop application is the core of the system. It runs on a PC next to the line, collects events from the line devices and keeps the production indicators up to date. It works fully offline inside the plant network.

> The source code is private because the application runs in production. This document covers the architecture, the technical decisions and how the application evolved. The `examples/` folder holds small, self-contained snippets written to illustrate those decisions; they are not extracted from the application.

## Requirements (Mar 2026)

- No internet on the shop floor: everything has to run on a closed local network.
- Read inspection results from industrial vision cameras over TCP.
- Run unattended on line PCs and TV panels.
- Never lose counted production after a power cut.

## Architecture decision (Apr 2026)

The Streamlit prototype depended on a Python environment and on spreadsheets filled in by hand. The next step is a self-contained desktop executable: one installer per line PC, all dependencies bundled, devices read directly.

A JavaFX proof of concept was tried first and dropped the same day because of runtime issues on the factory PCs. Electron with Node.js was chosen instead: a single installer, a web interface, and native access to serial ports and sockets.

## Evolution

### 1.0.0 (Jun 2026)

- Electron + Node.js desktop application with an offline web interface.
- Line data received from a direct TCP client in JSON.
