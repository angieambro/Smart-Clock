# Smartclock Arduino Project

This repository contains the starter firmware and hardware notes for an Arduino Due smart clock.

## Project Structure

- `src/` — main firmware source
- `include/` — pin assignments and shared definitions
- `docs/` — hardware notes and beginner code guide
- `platformio.ini` — Arduino Due settings and required libraries

## Build and Upload

Install PlatformIO, open this folder in VS Code, connect the Arduino Due, then build/upload the `due` environment. PlatformIO installs the libraries listed in `platformio.ini`.

The firmware uses the DS3231, DHT11, DFPlayer Mini, ST7735S TFT, rotary encoder, and motor driver. It shows the time, date, temperature, humidity, audio state, volume, and motor state on the screen.

Encoder actions: rotate to change DFPlayer volume, short press to play/pause track 1, and hold for 0.8 seconds to toggle the motor. The six LED outputs are currently initialized off; their behavior has not been specified yet.

Start with [docs/code-guide.md](docs/code-guide.md) for a plain-language explanation of the code. See [docs/hardware.md](docs/hardware.md) for the wiring, power rails, and hardware cautions.
