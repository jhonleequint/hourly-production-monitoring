# Changelog

Project timeline, newest first.

## 2026-11-06

- **Desktop app 3.1.0.** Remote access through a local HTTPS server on port 3000; legacy TCP and serial drivers archived.
- **Desktop app 3.1.1.** Classic 3.0.0 look restored, persistence in SQLite via WebAssembly (sql.js) and automatic event classification per station.
- **Desktop app 3.2.0.** Targets and production orders read from the planning spreadsheet, protected orders API and first station pairing flow.
- **Desktop app 3.2.1.** Shift and planned-stop management, history per sector and production installers.

## 2026-11-03

- **Packaging systems analysis.** Configuration of the packaging and warehouse systems studied to plan centralized production orders and serial numbers.

## 2026-10-14

- **Desktop app 3.0.0.** User manual, access control with password profiles and serial (COM) input from the ESP32 gateway.

## 2026-09-08

- **Desktop app 2.8.0.** Camera connection guide, 1 Hz heartbeat and daily storage in JSON files.

## 2026-08-20

- **Desktop app 2.5.0.** Local WOFF2 fonts for 100% offline operation and local TCP bridge utilities.

## 2026-08-05

- **Prototype integration.** Architecture of the line setup (ESP32 input, vision camera output, desktop app, database) and wiring documented.

## 2026-08-04

- **ESP-NOW gateway.** Receiver firmware that forwards station events to the desktop app over serial.

## 2026-08-03

- **Firmware V3[B].** ESP32 operator station sending production and defect events over ESP-NOW. The earlier ESP32 iterations built in Wokwi (V2 and V3[A]) were added alongside.
- **Reject counting PoC.** Side proof of concept in C#/WPF with OpenCV to count rejected boards on trays with a USB camera (not part of this repository).

## 2026-07-09

- **Desktop app 2.0.0.** Multiple lines at the same time, line-stopped detection that respects shifts, full-screen rotating TV mode and Mandarin translation.

## 2026-06-29

- **Desktop app 1.1.0.** JSON replaced by newline-delimited raw text over a standard TCP socket.

## 2026-06-08

- **Desktop app 1.0.0.** Migration to Electron and Node.js, offline web interface, data received from a direct TCP client in JSON.

## 2026-06-07

- **JavaFX proof of concept.** Desktop prototype in JavaFX, dropped the same day after runtime issues on the factory PCs.

## 2026-05-27

- **Dashboard restructure.** Data model for lines and occurrences: sector filter, quality (FQA, IPQC) and HR indicators, target attainment and scrap rate.

## 2026-04-20

- **Architecture.** Decision to move to a standalone desktop executable for line PCs and TV panels.

## 2026-03-18

- **Shop-floor requirements.** Communication protocols of the inspection cameras surveyed and a closed-network topology (no internet) defined.

## 2026-02-15

- **Shop-floor validation.** Dashboard evaluated on the line; the main gap found was the need for automatic, real-time data collection instead of manual spreadsheets.

## 2026-01-21

- **Dashboard prototype (Python/Streamlit).** Offline dashboard reading production spreadsheets, with UPPH, hourly target and an HR section.

## 2025-12-27

- **Firmware V1.** First operator station prototype on Arduino Uno, built and tested in Tinkercad: keypad login, LCD menu, ultrasonic part detection, servo reject, temperature alarm and RGB andon light.
