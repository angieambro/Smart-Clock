#pragma once

#include <Arduino.h>

#include "pins.h"  

namespace sunlamp {

// Settings: change these to whichever appropriate 
constexpr bool SUNLAMP_DEMO = true;      // true = quick demo, false = real schedule
constexpr uint8_t WAKE_HOUR = 7;         // sunrise starts at 07:00 ...
constexpr uint8_t WAKE_MINUTE = 0;
constexpr uint16_t RAMP_MINUTES = 30;    // ... gets to full brightness in 30 minutes
constexpr uint16_t HOLD_MINUTES = 30;    // ... stays at full for 30 minutes, then off
constexpr uint8_t MAX_BRIGHTNESS = 255;  // 0-255, lower it if the lamp is too bright
constexpr uint32_t DEMO_RAMP_MS = 5000;  // demo: 5 seconds up, then 5 seconds down
// The schedule must finish before midnight (07:00 + 30 + 30 minutes is fine).

// The last brightness that was sent to the LEDs (so we only write when it changes).
static uint8_t currentLevel = 0;

// LEDs look much brighter at low PWM values than the numbers suggest.
// Squaring the level makes the fade look smooth and even to the eye.
static uint8_t smoothLevel(uint8_t level) {
  return static_cast<uint8_t>((static_cast<uint16_t>(level) * level) / 255);
}

// Sets all six LEDs to the same brightness (0-255).
static void setLevel(uint8_t level) {
  if (level == currentLevel) {
    return;
  }
  currentLevel = level;
  const uint8_t pwm = smoothLevel(level);
  for (const uint8_t pin : LED_PINS) {
    analogWrite(pin, pwm);
  }
}

// Call once from setup().
static void begin() {
  for (const uint8_t pin : LED_PINS) {
    pinMode(pin, OUTPUT);
    analogWrite(pin, 0);
  }
  currentLevel = 0;
  SerialUSB.println(SUNLAMP_DEMO ? "Sunlamp: demo mode (fades every 10 s)"
                                 : "Sunlamp: schedule mode");
}

//   timeValid    - true when the RTC was found
//   secondsOfDay - the clock time in seconds since midnight (07:15:00 = 26100)
static void update(bool timeValid, uint32_t secondsOfDay) {
  if (SUNLAMP_DEMO) {
    // Fade up for DEMO_RAMP_MS, then down for DEMO_RAMP_MS, over and over.
    const uint32_t t = millis() % (2UL * DEMO_RAMP_MS);
    const uint32_t up = (t < DEMO_RAMP_MS) ? t : (2UL * DEMO_RAMP_MS - t);
    setLevel(static_cast<uint8_t>(up * MAX_BRIGHTNESS / DEMO_RAMP_MS));
    return;
  }

  if (!timeValid) {
    setLevel(0);  // no clock, so no schedule: keep the lamp off
    return;
  }

  const uint32_t start = WAKE_HOUR * 3600UL + WAKE_MINUTE * 60UL;
  const uint32_t rampSeconds = RAMP_MINUTES * 60UL;
  const uint32_t end = start + rampSeconds + HOLD_MINUTES * 60UL;

  if (secondsOfDay < start || secondsOfDay >= end) {
    setLevel(0);  // before sunrise or after the lamp's time is over
  } else if (secondsOfDay < start + rampSeconds) {
    setLevel(static_cast<uint8_t>(MAX_BRIGHTNESS * (secondsOfDay - start) / rampSeconds));
  } else {
    setLevel(MAX_BRIGHTNESS);  // full sunlight
  }
}

}  
