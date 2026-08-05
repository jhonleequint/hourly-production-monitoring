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

### 1.1.0 (Jun 2026)

- JSON replaced by newline-delimited raw text over a standard TCP socket, which is what the cameras send natively. A single read can carry half a line or several lines, so input is buffered until each newline. See [`examples/tcp-line-reader.js`](examples/tcp-line-reader.js).

### 2.0.0 (Jul 2026)

- Several lines monitored at the same time, with data fully isolated per line.
- Line-stopped detection that respects shift schedules. Input and output each have their own idle timer (20 s) and one never resets the other, so a jam where the input stops while the output keeps draining is still counted. See [`examples/downtime-timer.js`](examples/downtime-timer.js).
- Full-screen TV mode rotating between lines.
- Interface in Portuguese, English and Mandarin.

### Line integration prototype (Aug 2026)

![Prototype architecture](../docs/architecture-prototype.png)

ESP32 at the line input over serial, vision camera at the output over TCP, both read independently so a failure on one side never blocks the other. The firmware for the input side is in [`../firmware`](../firmware).
