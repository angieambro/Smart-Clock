# Hardware Notes

This document follows the project schematic and the clarified power and pin details. Keep the wiring and `include/pins.h` in agreement.

## Power Rails

- The battery pack is two Li-ion cells in series: about 7.4 V nominal and 8.4 V when fully charged.
- The battery `VCC` rail powers the Arduino Due `VIN` input and the input of the LM2596 buck converter.
- The Due regulates its own separate `3v3` rail for the TFT logic.
- The LM2596 output is a separate `3.3V` rail for the motor, DFPlayer VCC, and DHT11 VDD. Do not connect this rail to the Due's `3V3` pin.
- The grounds must be connected together so the signal levels have a shared reference.

Use a protected 2-cell battery pack and a charger designed for a 2S Li-ion pack. Confirm the LM2596 output is adjusted to 3.3 V before connecting the peripherals. The Due's `VIN` input is within its normal range at a 7.4 V nominal battery voltage; the pack voltage changes as it charges and discharges.

## Pin Map

| Component signal | Arduino Due connection |
| --- | --- |
| LEDs 1-6 | D1-D6, each through its 220 ohm resistor |
| DHT11 data | D7; DHT11 VDD is on the LM2596 3.3 V rail, with the shown 10 kohm pull-up |
| Motor transistor base | D9 through the 1 kohm resistor |
| RTC SDA/SCL | Due SDA/SCL (D20/D21) |
| DFPlayer RX/TX | Due TX1/RX1 (D18/D19), crossed TX-to-RX |
| TFT reset/DC/CS/backlight | D22/D24/D26/D28 |
| Encoder A/B/push | D30/D32/D34 |
| TFT SCL/SDA | Due ICSP SCK/MOSI (hardware SPI clock and data) |

The selected display listing specifies a 1.8-inch, 128x160 SPI TFT using an ST7735S controller. The project uses Adafruit_ST7735 for its graphics driver. The module's encoder and display are separate devices on the same board: encoder A/B/button are GPIO inputs, while the screen uses SPI.

## Motor Driver

The NPN transistor is a low-side switch: motor positive connects to the LM2596 3.3 V rail, motor negative to the transistor collector, and emitter to ground. D9 drives the base through 1 kohm. The code uses PWM on D9 with an initial duty of 200/255; a short encoder press does not affect the motor, while a press-and-hold toggles it.

The flyback diode goes across the motor, with its cathode (striped end) toward 3.3 V and its anode toward the transistor collector. Choose a diode and transistor rated for the motor's measured startup/stall current. Do not rely on the transistor voltage drop alone to protect the motor from overvoltage; check the motor voltage and current under load.

## Audio and Display

- Put audio on the DFPlayer microSD card as track 1. The firmware starts at volume 10 out of 30.
- Connect the speaker between DFPlayer `SPK1` and `SPK2`. These are bridged outputs; neither speaker lead goes to ground.
- TFT reset/DC/CS use D22/D24/D26. TFT SCL/SDA are SPI SCK/MOSI on the Due ICSP header.
- The Due cannot hardware-PWM D28. Firmware therefore sets the backlight pin on continuously. For brightness control, move the BLK wire to an available Due PWM pin and update `TFT_BACKLIGHT_PIN`.
- If the screen initializes but colors or image position are wrong, the module may use a different ST7735 tab/offset setting; adjust `display.initR(INITR_BLACKTAB)` in `src/main.cpp` after checking the display documentation.

## Due Pin Cautions

- D1 is also the Due programming-port UART TX pin. The firmware uses `SerialUSB` for debugging so normal serial messages do not use D1, but the LED connected to D1 can still load the programming UART during upload. If uploads fail, move LED1 to a free pin and update `LED_PINS` in `include/pins.h`.
- The Due GPIO signals are 3.3 V. Do not feed a 5 V signal into a Due input.
- The firmware's debug messages use the Due's native USB port. Select that port in the serial monitor after uploading; the programming USB port is used for upload.