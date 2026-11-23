# Firmware Code Guide

The root Arduino sketch integrates the master core, TFT, and DFPlayer modules.
It reads the RTC and DHT11, manages alarm and timer screens, and controls the
fan and audio alerts.

## Project Files

- `TalkieToaster.ino` initializes and updates the master core.
- `shared/pins.h` is the canonical Arduino Due pin map.
- `shared/system_state.h` defines the state exchanged by the modules.
- `01_angie_feng_master_core/` handles the RTC, DHT11, encoder, alarm, timer,
  temperature and humidity thresholds, automatic/manual motor PWM, and SG90
  servo.
- `03_faith_tft_ui/` renders the current screen.
- `05_binhao_audio/` sends the selected MP3 track commands to the DFPlayer.
- `02_nara_sun_lamp/` and `04_thang_alert_leds/` remain separate modules.

## Startup and Main Loop

Arduino calls `setup()` once. It starts native USB logging and initializes the
master core without waiting for a serial connection.

The root sketch updates the core, then sends the latest state to the display
and audio modules.

## Encoder and Modes

The Due calls `onEncoderChange()` on changes to both encoder channels. A
quadrature transition table filters invalid transitions and counts one detent
after four valid quarter-steps. The main loop copies and clears the volatile
detent count with interrupts briefly disabled.

The mode/reset button cycles directly through Home, Alarm, and Timer. In
Home, holding it toggles the manual fan request; holding the encoder opens the
controls guide. A short press of either button returns from the guide to Home.
In Alarm, rotation edits the selected hour or minute, a short encoder press
switches fields, and a long press toggles the alarm. Holding the mode/reset
button resets the alarm to 00:00 and disables it. In Timer, short encoder
presses select hours, minutes, then seconds. Rotation changes hours and
minutes in one-unit steps and seconds in 10-second steps, with a 24-hour
maximum; a long encoder press starts or pauses the timer. Holding the
mode/reset button resets the timer to 00:00:00.

## Alarm, Timer, and Sensors

The core reads the DS3231 periodically and updates the shared date/time. If
the RTC lost power, its time is initialized from the firmware build time. The
alarm triggers once per day at its configured hour and minute.

The DHT11 is read every two seconds. High/low temperature alerts activate at
35 C and 18 C. The fan starts at 33 C and stops when the temperature is at or
below 31 C and humidity is below 65%. High humidity activates at 70% and low
humidity at 20%; high humidity also requests the fan.

The countdown uses elapsed `millis()` time, so it continues without blocking
the encoder, clock, or other updates.

## Display, LEDs, and Audio

The master core drives motor PWM on D9 and sweeps the SG90 signal on D8
between 45 and 135 degrees while active, returning to 90 degrees when the motor
stops. DFPlayer audio uses `/MP3/0001.mp3` through `/MP3/0009.mp3` on its
microSD card; timer completions alternate between tracks 0001 and 0004.

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
