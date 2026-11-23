#include "core_logic.h"

#include "../shared/pins.h"
#include <DHT.h>
#include <RTClib.h>
#include <Servo.h>
#include <Wire.h>

namespace {
constexpr uint8_t DHT_TYPE = DHT11;
constexpr uint8_t MOTOR_PWM_DUTY = 200;
constexpr float TEMP_HIGH_THRESHOLD = 35.0f;
constexpr float TEMP_LOW_THRESHOLD = 18.0f;
constexpr float FAN_TEMP_ON_THRESHOLD = 33.0f;
constexpr float FAN_TEMP_OFF_THRESHOLD = 31.0f;
constexpr float HUMIDITY_HIGH_ON_THRESHOLD = 70.0f;
constexpr float HUMIDITY_HIGH_OFF_THRESHOLD = 65.0f;
constexpr float HUMIDITY_LOW_THRESHOLD = 20.0f;
constexpr uint32_t SENSOR_INTERVAL_MS = 2000;
constexpr uint32_t CLOCK_INTERVAL_MS = 250;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint32_t LONG_PRESS_MS = 800;
constexpr uint32_t SERVO_STEP_INTERVAL_MS = 20;
constexpr uint32_t SNOOZE_INTERVAL_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t TIMER_MINUTE_STEP_SECONDS = 60;
constexpr uint32_t MAX_TIMER_SECONDS = 24UL * 60UL * 60UL;

DHT dht(DHT_PIN, DHT_TYPE);
RTC_DS3231 rtc;
Servo fanServo;
volatile int16_t encoderSteps = 0;
volatile int8_t encoderQuarterSteps = 0;
volatile uint8_t encoderState = 0;
constexpr int8_t ENCODER_TRANSITIONS[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0};
struct ButtonState {
  uint8_t raw = HIGH;
  uint8_t stable = HIGH;
  uint32_t changedAt = 0;
  uint32_t pressedAt = 0;
  bool longPressHandled = false;
};
ButtonState encoderButton;
ButtonState modeButton;
uint32_t lastSensorReadAt = 0;
uint32_t lastClockReadAt = 0;
uint32_t lastTimerUpdateAt = 0;
uint32_t lastAlarmDateKey = 0;
uint32_t snoozeUntil = 0;
bool editingAlarmHour = true;
TimerField editingTimerField = TIMER_HOURS;
bool snoozeActive = false;
bool manualMotorRequested = false;
bool automaticFanRequested = false;
bool servoWasMoving = false;
uint8_t servoAngle = 90;
int8_t servoDirection = 1;
uint32_t lastServoStepAt = 0;
uint32_t lastSensorErrorAt = 0;

void onEncoderChange() {
  const uint8_t currentState =
      static_cast<uint8_t>((digitalRead(ENCODER_A_PIN) << 1) |
                           digitalRead(ENCODER_B_PIN));
  const uint8_t transition = static_cast<uint8_t>((encoderState << 2) |
                                                   currentState);
  encoderState = currentState;
  encoderQuarterSteps += ENCODER_TRANSITIONS[transition];

  if (encoderQuarterSteps >= 4) {
    ++encoderSteps;
    encoderQuarterSteps -= 4;
  } else if (encoderQuarterSteps <= -4) {
    --encoderSteps;
    encoderQuarterSteps += 4;
  }
}

void setMotor(SystemState& state, bool shouldRun) {
  state.motorRunning = shouldRun;
  analogWrite(MOTOR_PIN, shouldRun ? MOTOR_PWM_DUTY : 0);
  SerialUSB.println(shouldRun ? "Motor on" : "Motor off");
}

void updateMotorOutput(SystemState& state) {
  const bool shouldRun = manualMotorRequested || automaticFanRequested;
  if (state.motorRunning != shouldRun) {
    setMotor(state, shouldRun);
  }
}

void dismissAlerts(SystemState& state) {
  if (state.alarmRinging) {
    state.alarmRinging = false;
    snoozeActive = false;
  }
  if (state.timerFinished) {
    state.timerFinished = false;
  }
  state.uiMode = MODE_NORMAL;
}

enum class ButtonEvent : uint8_t {
  NONE,
  SHORT_PRESS,
  LONG_PRESS
};

ButtonEvent readButton(uint8_t pin, ButtonState& button, uint32_t now) {
  const uint8_t reading = digitalRead(pin);
  if (reading != button.raw) {
    button.raw = reading;
    button.changedAt = now;
  }

  if (button.raw != button.stable &&
      now - button.changedAt >= BUTTON_DEBOUNCE_MS) {
    button.stable = button.raw;
    if (button.stable == LOW) {
      button.pressedAt = now;
      button.longPressHandled = false;
    } else if (!button.longPressHandled) {
      return ButtonEvent::SHORT_PRESS;
    }
  }

  if (button.stable == LOW && !button.longPressHandled &&
      now - button.pressedAt >= LONG_PRESS_MS) {
    button.longPressHandled = true;
    return ButtonEvent::LONG_PRESS;
  }
  return ButtonEvent::NONE;
}

void handleModeButton(SystemState& state, ButtonEvent event) {
  if (event == ButtonEvent::LONG_PRESS) {
    if (state.uiMode == MODE_NORMAL) {
      manualMotorRequested = !manualMotorRequested;
      updateMotorOutput(state);
    } else if (state.uiMode == MODE_SET_ALARM) {
      state.alarmHour = 0;
      state.alarmMinute = 0;
      state.alarmEnabled = false;
      editingAlarmHour = true;
    } else if (state.uiMode == MODE_SET_TIMER) {
      state.timerRunning = false;
      state.timerFinished = false;
      state.timerSecondsLeft = 0;
      state.timerRemainingMs = 0;
      editingTimerField = TIMER_HOURS;
    }
    return;
  }

  if (event != ButtonEvent::SHORT_PRESS) {
    return;
  }
  if (state.uiMode == MODE_HELP) {
    state.uiMode = MODE_NORMAL;
    return;
  }
  if (state.alarmRinging || state.timerFinished) {
    return;
  }

  switch (state.uiMode) {
    case MODE_NORMAL:
      state.uiMode = MODE_SET_ALARM;
      editingAlarmHour = true;
      break;
    case MODE_SET_ALARM:
      state.uiMode = MODE_SET_TIMER;
      editingTimerField = TIMER_HOURS;
      break;
    case MODE_SET_TIMER:
      state.uiMode = MODE_NORMAL;
      break;
    case MODE_HELP:
      state.uiMode = MODE_NORMAL;
      break;
    case MODE_ALERT_ACTIVE:
      break;
  }
}

void handleEncoderShortPress(SystemState& state) {
  switch (state.uiMode) {
    case MODE_NORMAL:
      break;
    case MODE_SET_ALARM:
      editingAlarmHour = !editingAlarmHour;
      state.alarmEditingHour = editingAlarmHour;
      break;
    case MODE_SET_TIMER:
      editingTimerField = static_cast<TimerField>(
          (editingTimerField + 1) % 3);
      state.timerEditingField = editingTimerField;
      break;
    case MODE_HELP:
      state.uiMode = MODE_NORMAL;
      break;
    case MODE_ALERT_ACTIVE:
      dismissAlerts(state);
      break;
  }
}

void handleEncoderLongPress(SystemState& state) {
  switch (state.uiMode) {
    case MODE_NORMAL:
      state.uiMode = MODE_HELP;
      break;
    case MODE_SET_ALARM:
      state.alarmEnabled = !state.alarmEnabled;
      break;
    case MODE_SET_TIMER:
      if (state.timerRunning) {
        state.timerRemainingMs = state.timerSecondsLeft * 1000UL;
        state.timerRunning = false;
      } else {
        if (state.timerSecondsLeft == 0) {
          state.timerSecondsLeft = TIMER_MINUTE_STEP_SECONDS;
        }
        state.timerRemainingMs = state.timerSecondsLeft * 1000UL;
        state.timerRunning = true;
        state.timerFinished = false;
        lastTimerUpdateAt = millis();
      }
      break;
    case MODE_HELP:
      break;
    case MODE_ALERT_ACTIVE:
      if (state.alarmRinging) {
        state.alarmRinging = false;
        snoozeUntil = millis() + SNOOZE_INTERVAL_MS;
        snoozeActive = true;
        state.uiMode = MODE_NORMAL;
      } else {
        dismissAlerts(state);
      }
      break;
  }
}

void handleEncoder(SystemState& state) {
  noInterrupts();
  const int16_t steps = encoderSteps;
  encoderSteps = 0;
  interrupts();

  if (steps != 0) {
    if (state.uiMode == MODE_SET_ALARM) {
      if (editingAlarmHour) {
        state.alarmHour = static_cast<uint8_t>(
            (static_cast<int16_t>(state.alarmHour) + steps % 24 + 24) % 24);
      } else {
        state.alarmMinute = static_cast<uint8_t>(
            (static_cast<int16_t>(state.alarmMinute) + steps % 60 + 60) % 60);
      }
    } else if (state.uiMode == MODE_SET_TIMER && !state.timerRunning) {
      int32_t hours = static_cast<int32_t>(state.timerSecondsLeft / 3600UL);
      int32_t minutes =
          static_cast<int32_t>((state.timerSecondsLeft % 3600UL) / 60UL);
      int32_t seconds = static_cast<int32_t>(state.timerSecondsLeft % 60UL);

      switch (editingTimerField) {
        case TIMER_HOURS:
          hours = (hours + steps) % 25;
          if (hours < 0) {
            hours += 25;
          }
          if (hours == 24) {
            minutes = 0;
            seconds = 0;
          }
          break;
        case TIMER_MINUTES:
          if (hours < 24) {
            minutes = (minutes + steps) % 60;
            if (minutes < 0) {
              minutes += 60;
            }
          }
          break;
        case TIMER_SECONDS: {
          if (hours < 24) {
            int32_t secondStep = seconds / 10;
            secondStep = (secondStep + steps) % 6;
            if (secondStep < 0) {
              secondStep += 6;
            }
            seconds = secondStep * 10;
          }
          break;
        }
      }

      state.timerSecondsLeft = static_cast<uint32_t>(
          hours * 3600L + minutes * 60L + seconds);
      if (!state.timerRunning) {
        state.timerRemainingMs = state.timerSecondsLeft * 1000UL;
      }
    }
  }

  const uint32_t now = millis();
  handleModeButton(state, readButton(MODE_BUTTON_PIN, modeButton, now));
  const ButtonEvent encoderEvent =
      readButton(ENCODER_PUSH_PIN, encoderButton, now);
  if (encoderEvent == ButtonEvent::SHORT_PRESS) {
    handleEncoderShortPress(state);
  } else if (encoderEvent == ButtonEvent::LONG_PRESS) {
    handleEncoderLongPress(state);
  }
}

void readClock(SystemState& state, uint32_t now) {
  if (!state.rtcReady || now - lastClockReadAt < CLOCK_INTERVAL_MS) {
    return;
  }
  lastClockReadAt = now;

  const DateTime current = rtc.now();
  state.currentHour = current.hour();
  state.currentMinute = current.minute();
  state.currentSecond = current.second();
  state.currentYear = current.year();
  state.currentMonth = current.month();
  state.currentDay = current.day();

  const uint32_t dateKey = static_cast<uint32_t>(current.year()) * 10000UL +
                           current.month() * 100UL + current.day();
  if (state.alarmEnabled && !state.alarmRinging &&
      state.currentHour == state.alarmHour &&
      state.currentMinute == state.alarmMinute &&
      lastAlarmDateKey != dateKey) {
    lastAlarmDateKey = dateKey;
    state.alarmRinging = true;
  }
}

void readSensors(SystemState& state, uint32_t now) {
  if (now - lastSensorReadAt < SENSOR_INTERVAL_MS) {
    return;
  }
  lastSensorReadAt = now;

  float humidity = NAN;
  float temperature = NAN;
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    if (!isnan(humidity) && !isnan(temperature)) {
      break;
    }
    delay(150);
  }
  if (isnan(humidity) || isnan(temperature)) {
    state.dhtReady = false;
    state.temperatureC = NAN;
    state.humidityPercent = NAN;
    if (now - lastSensorErrorAt >= SENSOR_INTERVAL_MS) {
      SerialUSB.println("DHT11 read failed; check its wiring");
      lastSensorErrorAt = now;
    }
    state.highTempAlert = false;
    state.lowTempAlert = false;
    state.highHumidityAlert = false;
    state.lowHumidityAlert = false;
    return;
  }

  state.dhtReady = true;
  state.temperatureC = temperature;
  state.humidityPercent = humidity;
  state.highTempAlert = temperature >= TEMP_HIGH_THRESHOLD;
  state.lowTempAlert = temperature <= TEMP_LOW_THRESHOLD;
  state.highHumidityAlert = humidity >= HUMIDITY_HIGH_ON_THRESHOLD;
  state.lowHumidityAlert = humidity <= HUMIDITY_LOW_THRESHOLD;
}

void updateAutomaticFan(SystemState& state) {
  if (state.dhtReady) {
    const bool hotEnough = state.temperatureC >= FAN_TEMP_ON_THRESHOLD;
    const bool humidityHigh = state.humidityPercent >= HUMIDITY_HIGH_ON_THRESHOLD;
    const bool temperatureCooled = state.temperatureC <= FAN_TEMP_OFF_THRESHOLD;
    const bool humidityDropped = state.humidityPercent < HUMIDITY_HIGH_OFF_THRESHOLD;

    if (!automaticFanRequested && (hotEnough || humidityHigh)) {
      automaticFanRequested = true;
    } else if (automaticFanRequested && temperatureCooled && humidityDropped) {
      automaticFanRequested = false;
    }
  }
  updateMotorOutput(state);
}

void updateTimer(SystemState& state, uint32_t now) {
  if (!state.timerRunning) {
    return;
  }

  const uint32_t elapsed = now - lastTimerUpdateAt;
  lastTimerUpdateAt = now;
  if (elapsed >= state.timerRemainingMs) {
    state.timerRemainingMs = 0;
    state.timerSecondsLeft = 0;
    state.timerRunning = false;
    state.timerFinished = true;
  } else {
    state.timerRemainingMs -= elapsed;
    state.timerSecondsLeft = (state.timerRemainingMs + 999UL) / 1000UL;
  }
}

void updateSnooze(SystemState& state, uint32_t now) {
  if (snoozeActive && static_cast<int32_t>(now - snoozeUntil) >= 0) {
    snoozeActive = false;
    state.alarmRinging = true;
  }
}

void updateServo(SystemState& state, uint32_t now) {
  if (!state.motorRunning) {
    if (servoWasMoving) {
      servoAngle = 90;
      fanServo.write(servoAngle);
      servoWasMoving = false;
    }
    return;
  }

  if (now - lastServoStepAt < SERVO_STEP_INTERVAL_MS) {
    return;
  }
  lastServoStepAt = now;
  servoWasMoving = true;

  const int16_t nextAngle = static_cast<int16_t>(servoAngle) + servoDirection * 2;
  if (nextAngle >= 135) {
    servoAngle = 135;
    servoDirection = -1;
  } else if (nextAngle <= 45) {
    servoAngle = 45;
    servoDirection = 1;
  } else {
    servoAngle = static_cast<uint8_t>(nextAngle);
  }
  fanServo.write(servoAngle);
}
}

void initCore(SystemState& state) {
  Wire1.begin();
  state.rtcReady = false;
  for (uint8_t attempt = 0; attempt < 5 && !state.rtcReady; attempt++) {
    state.rtcReady = rtc.begin(&Wire1);
    if (!state.rtcReady) {
      delay(100);
    }
  }
  if (!state.rtcReady) {
    SerialUSB.println("DS3231 not found; check SDA, SCL, and power");
  } else if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    SerialUSB.println("RTC time set from firmware build time");
  }

  dht.begin();
  state.dhtReady = true;

  pinMode(MOTOR_PIN, OUTPUT);
  analogWriteResolution(8);
  analogWrite(MOTOR_PIN, 0);
  pinMode(SERVO_PIN, OUTPUT);
  fanServo.attach(SERVO_PIN);
  fanServo.write(90);

  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(ENCODER_PUSH_PIN, INPUT_PULLUP);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  encoderState = static_cast<uint8_t>((digitalRead(ENCODER_A_PIN) << 1) |
                                      digitalRead(ENCODER_B_PIN));
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), onEncoderChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), onEncoderChange, CHANGE);

  lastSensorReadAt = millis() - SENSOR_INTERVAL_MS;
  lastClockReadAt = millis() - CLOCK_INTERVAL_MS;
  lastTimerUpdateAt = millis();
}

void updateCore(SystemState& state) {
  const uint32_t now = millis();
  handleEncoder(state);
  state.alarmEditingHour = editingAlarmHour;
  state.timerEditingField = editingTimerField;
  readClock(state, now);
  readSensors(state, now);
  updateAutomaticFan(state);
  updateTimer(state, now);
  updateSnooze(state, now);
  updateServo(state, now);

  if (state.alarmRinging || state.timerFinished) {
    state.uiMode = MODE_ALERT_ACTIVE;
  } else if (state.uiMode == MODE_ALERT_ACTIVE) {
    state.uiMode = MODE_NORMAL;
  }
}
