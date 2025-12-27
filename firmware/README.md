# Firmware

Operator station prototypes, from a first Arduino build to the ESP32 version that talks to the desktop application.

## V1 - Arduino Uno (Tinkercad)

![Arduino V1 in Tinkercad](images/arduino-v1-tinkercad.png)

- 4x4 keypad login and LCD menu (production, setup, stops).
- Production pulses counted through an interrupt.
- Ultrasonic part detection and servo reject.
- TMP36 temperature alarm and RGB andon light.
- Automatic lock after 8 h, 2 min menu timeout and lock after end of shift.

Simulation: [Tinkercad](https://www.tinkercad.com/things/6rfiOlwrk9n-sistema-automatizado-de-inspecao-e-monitoramento-v1)
