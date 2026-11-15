# Talkie Toaster — Master Firmware Scope

The executable firmware currently contains the root `TalkieToaster.ino`, its
PlatformIO bridge in `src/main.cpp`, the core module in
`01_angie_feng_master_core/`, and shared definitions in `shared/`. Teammate
folders `02_nara_sun_lamp/`, `03_faith_tft_ui/`, `04_thang_alert_leds/`, and
`05_binhao_audio/` remain as empty implementation scaffolds; their `.cpp` files
are excluded from the master build until their owners are ready.

## Hardware constraints

- The Arduino Due uses 3.3 V logic and is not 5 V tolerant.
- Use `SerialUSB` for logging, but never block setup waiting for a serial
  connection.
- Phase 1 power is through the Due Micro-USB port. Do not use a 5 V power
  bank on the barrel jack.
- During Phase 1, keep the DC motor, SG90 servo, and DFPlayer disconnected.
  Do not power high-current devices from the Due's 3V3/5V pins.
- For Phase 2, verify the external buck converter output before attaching
  loads, and use a common ground.
- Preserve the canonical pin assignments in `shared/pins.h`.

## Master core behavior

- Read DS3231 and DHT11 sensors using non-blocking intervals. Continue if
  either device is unavailable.
- Evaluate an enabled alarm against RTC time, a countdown timer, and high/low
  temperature thresholds.
- Use interrupt-driven encoder input and debounced button handling.
- Double-click cycles through normal, alarm setup, and timer setup. In alarm
  setup, rotation edits the selected hour or minute, short press changes the
  selected field, and long press enables the alarm. In timer setup, rotation
  changes the duration in 15-second increments, short press starts/pauses,
  and long press exits.
- Long press toggles the motor in normal mode. While an alarm is active,
  short press dismisses it and long press snoozes it for five minutes.
- Drive the motor output on D9 and sweep the SG90 signal on D8 from 45 to
  135 degrees while the motor is running; return to 90 degrees when stopped.
- Use `millis()` for timing. Do not use `delay()` in the main loop or update
  functions.

## Files

```text
TalkieToaster.ino
src/main.cpp
01_angie_feng_master_core/
  core_logic.h
  core_logic.cpp
  test_core.ino
shared/
  pins.h
  system_state.h
02_nara_sun_lamp/
  sun_lamp.h
  sun_lamp.cpp
03_faith_tft_ui/
  tft_ui.h
  tft_ui.cpp
04_thang_alert_leds/
  alert_leds.h
  alert_leds.cpp
05_binhao_audio/
  audio_player.h
  audio_player.cpp
```
