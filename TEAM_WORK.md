# Team Work Plan

This document records module ownership and the current integration boundaries.
The master firmware builds independently; teammate module implementations are
not yet called from `TalkieToaster.ino`.

## Owners and responsibilities

| Owner | Folder | Work to complete |
| --- | --- | --- |
| Angie & Feng | `01_angie_feng_master_core/` | Own hardware integration, shared state and pin assignments, RTC/DHT reads, encoder state machine, alarm and countdown logic, temperature thresholds, motor output, and servo sweep. Integrate and review teammate modules. |
| Nara | `02_nara_sun_lamp/` | Implement non-blocking time-of-day LED brightness using RTC data; turn the lamp off if RTC data is unavailable. |
| Faith | `03_faith_tft_ui/` | Implement the ST7735 layout for clock/date, sensors, alarm/timer, and system status. Clear the display only in initialization and redraw text with a background color. |
| Thang | `04_thang_alert_leds/` | Implement non-blocking LED patterns for ringing alarms and completed timers. Coordinate LED priority with the master integration. |
| Binhao Chen | `05_binhao_audio/` | Implement DFPlayer control on `Serial1` and scalable audio events for alarm, timer done, high temperature, and low temperature tracks. |

## Shared files and integration

- `shared/pins.h` is the canonical pin map.
- `shared/system_state.h` is the shared state contract. Ask Angie and Feng
  before changing shared fields or interfaces.
- Each owner implements only their module folder. Avoid including another
  owner's module directly; use shared state and declared interfaces.
- The `.cpp` files in folders `02`-`05` are placeholders. Their folders are
  excluded from the PlatformIO build until the master leads integrate them.
- Standalone module test sketches can be added by each owner in their own
  folder; keep them separate from the integrated PlatformIO build.

## Development checklist

1. Read `README.md`, `docs/hardware.md`, and this ownership plan before wiring
   or changing an interface.
2. Keep Arduino Due signal levels at 3.3 V. During Phase 1, use the Due
   Micro-USB port and leave the motor, servo, and DFPlayer disconnected.
3. Use `millis()` for timing; do not block with `delay()` or wait for serial
   during startup.
4. Build the current master with `py -m platformio run -e due`.
5. Test module behavior independently, then coordinate integration with Angie
   and Feng before adding its source files to `platformio.ini` and
   `TalkieToaster.ino`.
