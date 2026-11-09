# Firmware Code Guide

This guide explains the first Smartclock firmware in plain language. The program is written in C++ for the Arduino Due and uses small library objects to talk to the clock, sensor, audio player, and screen.

## Project Files

- `src/main.cpp` contains the program: startup, component setup, button handling, sensor reads, and screen updates.
- `include/pins.h` gives names to the Due pins. For example, code can use `MOTOR_PIN` instead of remembering that the motor transistor is on D9.
- `platformio.ini` tells PlatformIO which board and libraries to use.
- `docs/hardware.md` explains how the code's pin names connect to the schematic.

## What Happens at Startup

Arduino calls `setup()` once after power is applied or the board is reset. It starts the native USB serial connection for debug messages, configures the LEDs and motor output, and sets the encoder pins to `INPUT_PULLUP`. A pull-up makes an unpressed switch read HIGH; pressing the button connects it to ground, so it reads LOW.

The program then starts the RTC over I2C, the DFPlayer over `Serial1`, and the display over hardware SPI. If the RTC says it lost power, its time is initialized from the date and time when this firmware was compiled. The display is rotated to landscape and the backlight is switched on.

## What Happens Repeatedly

After `setup()`, Arduino calls `loop()` again and again. Each pass does three short jobs:

1. `handleEncoder()` collects rotation events and checks the push-button.
2. `readSensors()` reads the DHT11 no more than once every two seconds.
3. `updateScreen()` refreshes the display once per second.

The code uses `millis()` for timing instead of pausing with long `delay()` calls. That leaves time for the encoder and the other components to keep working.

## Encoder and Button

The Due calls `onEncoderTurn()` from a hardware interrupt when encoder A changes from HIGH to LOW. The function checks encoder B to decide which direction the knob moved. It only records the movement; `handleEncoder()` applies the volume change later in the normal loop.

`encoderSteps` is marked `volatile` because an interrupt can change it without warning while the main program is running. The code briefly disables interrupts while copying and clearing that value, then turns interrupts back on. This prevents a rotation from being partly lost during the copy.

The push-button is checked by polling. `BUTTON_DEBOUNCE_MS` ignores very fast electrical changes caused by the physical switch. A short press tells the DFPlayer to play or pause track 1. Holding the button at least 0.8 seconds and then releasing it toggles the motor.

## Sensors, Clock, and Screen

`readSensors()` asks the DHT library for temperature and humidity. DHT11 readings can fail, so the program checks for invalid `NaN` values and leaves placeholders on the display when there is no valid reading.

`drawScreen()` clears and redraws the 128x160 display in landscape orientation. It shows the DS3231 time/date, the latest temperature and humidity, the audio state and volume, and whether the motor is on. `updateScreen()` uses a timer so drawing happens once per second rather than on every pass through `loop()`.

The Amazon listing identifies the display controller as ST7735S. `Adafruit_ST7735` sends graphics commands over SPI; `Adafruit_GFX` supplies common drawing and text functions used by that driver. The `INITR_BLACKTAB` setting handles one common ST7735 panel layout. If the display has shifted colors or a misaligned image, its panel may need a different tab/offset setting.

## Audio and Motor

`playerReady` records whether the DFPlayer answered during startup. The program only sends audio commands if it is ready. `volume` is limited to 0 through 30, and the starting level is deliberately set to 10.

`setMotor()` controls the transistor, not the motor directly. D9 sends a PWM signal through the 1 kohm base resistor. `MOTOR_PWM_DUTY` sets the on-speed from 0 to 255; changing the constant changes the duty cycle. The motor's supply current comes from the LM2596 rail, not from the Due GPIO pin.

## C++ Terms Used Here

- A `#include` line makes a library or project header available to this file.
- A function is a named group of instructions. For example, `readSensors()` is called from `loop()` when sensor data should be refreshed.
- A variable stores a value that can change, such as `volume` or `motorRunning`.
- A `const` value is a setting that the program does not change while running, such as a pin number or timing interval.
- `bool` stores either `true` or `false`; `uint8_t` stores a small non-negative number; `float` can store decimal readings.
- `if` chooses which instructions run based on a condition. For example, audio commands are skipped if `playerReady` is false.
- `millis()` is the number of milliseconds since startup. Subtracting an earlier value from it checks whether enough time has passed without stopping the program.

## Safe Changes to Start With

- To change a pin, edit `include/pins.h` and make the same wiring change; do not assign one pin to two components.
- To change the initial audio level, edit `STARTING_VOLUME` in `src/main.cpp`.
- To change motor speed, adjust `MOTOR_PWM_DUTY` between 0 and 255, then verify the motor voltage and current physically.
- Build with `py -m platformio run` before uploading. The Due's native USB port is used for debug messages; its programming port is used to upload.

The six LEDs are initialized off but do not yet have a programmed pattern. Add their intended behavior only after deciding what they should indicate.