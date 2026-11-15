# Firmware Code Guide

The Talkie Toaster firmware currently runs only the master core. The root
Arduino sketch starts the core, which reads sensors and manages alarm, timer,
encoder, and actuator behavior. Teammate module folders remain as implementation
scaffolds and are not yet integrated into the main loop.

## Project Files

- `TalkieToaster.ino` initializes and updates the master core.
- `shared/pins.h` is the canonical Arduino Due pin map.
- `shared/system_state.h` defines the state exchanged by the modules.
- `01_angie_feng_master_core/` handles the RTC, DHT11, encoder, alarm, timer,
  temperature thresholds, motor PWM, and SG90 servo.
- `02_nara_sun_lamp/`, `03_faith_tft_ui/`, `04_thang_alert_leds/`, and
  `05_binhao_audio/` contain owner-ready headers and empty `.cpp` scaffolds.

## Startup and Main Loop

Arduino calls `setup()` once. It starts native USB logging and initializes the
master core without waiting for a serial connection.

The root sketch delegates its work to the master core. Teammate modules are
not called or compiled in the master build until they are implemented.

## Encoder and Modes

The Due calls `onEncoderTurn()` from a hardware interrupt when encoder A falls.
That function records the direction; the main loop copies and clears the
volatile count with interrupts briefly disabled.

In normal mode, a long press toggles the motor. Double-click cycles between
normal mode, alarm setup, and timer setup. In alarm setup, rotation edits the
selected hour or minute and short press switches fields; long press enables
the alarm. In timer setup, rotation changes the duration in 15-second steps,
short press starts/pauses, and long press exits. Short press dismisses an
active alert; long press snoozes an alarm for five minutes. Audio, volume, and
display controls are not included in this master-only firmware.

## Alarm, Timer, and Sensors

The core reads the DS3231 periodically and updates the shared date/time. If
the RTC lost power, its time is initialized from the firmware build time. The
alarm triggers once per day at its configured hour and minute.

The DHT11 is read every two seconds. Invalid readings are reported. Temperature
alerts use 30 C and 15 C as initial high/low thresholds.

The countdown uses elapsed `millis()` time, so it continues without blocking
the encoder, clock, or other updates.

## Display, LEDs, and Audio

The master core drives motor PWM on D9 and sweeps the SG90 signal on D8
between 45 and 135 degrees while active, returning to 90 degrees when the motor
stops. TFT, sun-lamp, alert LED, and DFPlayer implementations remain for the
teammates.

## Safe Changes and Testing

- Change pin assignments only in `shared/pins.h`.
- Adjust the temperature thresholds in
  `01_angie_feng_master_core/core_logic.cpp`.
- Build the master firmware with PlatformIO before uploading.
- For Phase 1, power the Due from 5 V Micro-USB; leave the motor, servo, and
  DFPlayer disconnected. Do not apply 5 V to Due signal pins or power
  high-current loads from the Due's headers.
- Read [hardware.md](hardware.md) and [COPILOT_MASTER_SPEC.md](../COPILOT_MASTER_SPEC.md)
  before wiring or changing the master core.
