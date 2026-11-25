#include "alert_leds.h"

#include "../shared/pins.h"

namespace {
// Thresholds aligned with core_logic.cpp, with hysteresis to prevent rapid flickering
constexpr float TEMP_HIGH_ON  = 35.0f;  // Turn on warning at >= 35.0 C
constexpr float TEMP_HIGH_OFF = 33.5f;  // Turn off warning at <= 33.5 C

constexpr float HUM_HIGH_ON   = 70.0f;  // Turn on warning at >= 70.0 %
constexpr float HUM_HIGH_OFF  = 65.0f;  // Turn off warning at <= 65.0 %

// Blink intervals (ms)
constexpr uint32_t FAST_BLINK_MS = 150;
constexpr uint32_t SLOW_BLINK_MS = 600;

bool tempWarningActive = false;
bool humWarningActive = false;
bool wasAlertActive = false;

uint32_t lastBlinkAt = 0;
bool ledToggleState = false;
uint8_t chaseIndex = 0;

// Use analogWrite(0 / 255) so PWM pin state stays compatible with Nara's sun lamp
void turnOffAllLeds() {
  for (const uint8_t pin : LED_PINS) {
    analogWrite(pin, 0);
  }
}

void setAllLeds(bool isOn) {
  const uint8_t pwm = isOn ? 255 : 0;
  for (const uint8_t pin : LED_PINS) {
    analogWrite(pin, pwm);
  }
}

void checkEnvironment(const SystemState& state) {
  if (!state.dhtReady) {
    tempWarningActive = false;
    humWarningActive = false;
    return;
  }

  if (!isnan(state.temperatureC)) {
    if (state.temperatureC >= TEMP_HIGH_ON) {
      tempWarningActive = true;
    } else if (state.temperatureC <= TEMP_HIGH_OFF) {
      tempWarningActive = false;
    }
  }

  if (!isnan(state.humidityPercent)) {
    if (state.humidityPercent >= HUM_HIGH_ON) {
      humWarningActive = true;
    } else if (state.humidityPercent <= HUM_HIGH_OFF) {
      humWarningActive = false;
    }
  }
}
}  // namespace

void initAlertLeds() {
  for (const uint8_t pin : LED_PINS) {
    pinMode(pin, OUTPUT);
    analogWrite(pin, 0);
  }
}

void updateAlertLeds(const SystemState& state) {
  const uint32_t now = millis();
  checkEnvironment(state);

  // Priority 1: Alarm ringing -> fast blink all 6 LEDs every 150 ms
  if (state.alarmRinging) {
    wasAlertActive = true;
    if (now - lastBlinkAt >= FAST_BLINK_MS) {
      lastBlinkAt = now;
      ledToggleState = !ledToggleState;
      setAllLeds(ledToggleState);
    }
    return;
  }

  // Priority 2: Timer finished -> chase LEDs from pin 1 to pin 6 every 150 ms
  if (state.timerFinished) {
    wasAlertActive = true;
    if (now - lastBlinkAt >= FAST_BLINK_MS) {
      lastBlinkAt = now;
      turnOffAllLeds();
      analogWrite(LED_PINS[chaseIndex], 255);

      chaseIndex++;
      if (chaseIndex >= 6) {
        chaseIndex = 0;
      }
    }
    return;
  }

  // Priority 3: High temperature or humidity -> slow blink outer 2 LEDs (pins 1 and 6)
  if (tempWarningActive || humWarningActive) {
    wasAlertActive = true;
    if (now - lastBlinkAt >= SLOW_BLINK_MS) {
      lastBlinkAt = now;
      ledToggleState = !ledToggleState;

      turnOffAllLeds();
      if (ledToggleState) {
        analogWrite(LED_PINS[0], 255);
        analogWrite(LED_PINS[5], 255);
      }
    }
    return;
  }

  // No active alerts: clear LEDs once on transition, then yield control to Sunlamp
  if (wasAlertActive) {
    turnOffAllLeds();
    wasAlertActive = false;
  }
  ledToggleState = false;
  chaseIndex = 0;
}
