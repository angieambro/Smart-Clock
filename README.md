# Smartclock Arduino Project

This repository contains modular Arduino Due firmware for the Talkie Toaster smart clock and desk assistant.

## Project Structure

- `TalkieToaster.ino` — integrated entry point
- `src/main.cpp` — PlatformIO bridge that compiles the root sketch
- `shared/` — canonical pin map and master state
- `01_angie_feng_master_core/` — clock, sensor, alarm, timer, encoder, motor, and servo logic
- `02_nara_sun_lamp/` — implementation scaffold for Nara
- `03_faith_tft_ui/` — implementation scaffold for Faith
- `04_thang_alert_leds/` — implementation scaffold for Thang
- `05_binhao_audio/` — implementation scaffold for Binhao
- `docs/` — hardware notes and beginner code guide
- `platformio.ini` — Arduino Due settings and required libraries
- `COPILOT_MASTER_SPEC.md` — project requirements and integration contract
- `TEAM_WORK.md` — team ownership and development workflow

## Build and Upload

The integrated firmware reads the DS3231 RTC and DHT11, drives the TFT and
encoder, automatically runs the fan for hot/high-humidity conditions, and
plays the configured DFPlayer tracks when the audio module and microSD card
are connected. The mode/reset button cycles through Home, Alarm, and Timer screens.

Build with `py -m platformio run -e due`. See
[TEAM_WORK.md](TEAM_WORK.md) for individual assignments and integration steps.

Start with [docs/code-guide.md](docs/code-guide.md) for a plain-language explanation. Read [docs/hardware.md](docs/hardware.md) before wiring; in particular, Phase 1 must use the Due Micro-USB port and leave high-current loads disconnected.
