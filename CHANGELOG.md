# Changelog

Project timeline, newest first.

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
