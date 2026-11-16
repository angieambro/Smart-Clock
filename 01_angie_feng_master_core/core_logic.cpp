#include "core_logic.h"

#include "../shared/pins.h"
#include <DHT.h>
#include <RTClib.h>
#include <Servo.h>
#include <Wire.h>

namespace {
constexpr uint8_t DHT_TYPE = DHT11;
constexpr uint8_t MOTOR_PWM_DUTY = 200;
constexpr float TEMP_HIGH_THRESHOLD = 30.0f;
constexpr float TEMP_LOW_THRESHOLD = 15.0f;
constexpr uint32_t SENSOR_INTERVAL_MS = 2000;
constexpr uint32_t CLOCK_INTERVAL_MS = 250;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint32_t DOUBLE_CLICK_MS = 350;
constexpr uint32_t LONG_PRESS_MS = 800;
constexpr uint32_t SERVO_STEP_INTERVAL_MS = 20;
constexpr uint32_t SNOOZE_INTERVAL_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t TIMER_STEP_SECONDS = 15;
constexpr uint32_t MAX_TIMER_SECONDS = 24UL * 60UL * 60UL;

DHT dht(DHT_PIN, DHT_TYPE);
RTC_DS3231 rtc;
Servo fanServo;
volatile int16_t encoderSteps = 0;
uint8_t rawButtonState = HIGH;
uint8_t stableButtonState = HIGH;
uint32_t buttonStateChangedAt = 0;
uint32_t buttonPressedAt = 0;
uint32_t lastSensorReadAt = 0;
uint32_t lastClockReadAt = 0;
uint32_t lastTimerUpdateAt = 0;
uint32_t lastAlarmDateKey = 0;
uint32_t snoozeUntil = 0;
uint32_t pendingClickAt = 0;
bool longPressHandled = false;
bool pendingClick = false;
bool editingAlarmHour = true;
bool snoozeActive = false;
bool servoWasMoving = false;
uint8_t servoAngle = 90;
int8_t servoDirection = 1;
uint32_t lastServoStepAt = 0;
uint32_t lastSensorErrorAt = 0;

void onEncoderTurn() {
  encoderSteps += digitalRead(ENCODER_B_PIN) == HIGH ? 1 : -1;
}

void setMotor(SystemState& state, bool shouldRun) {
  state.motorRunning = shouldRun;
  analogWrite(MOTOR_PIN, shouldRun ? MOTOR_PWM_DUTY : 0);
  SerialUSB.println(shouldRun ? "Motor on" : "Motor off");
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

void handleShortPress(SystemState& state) {
  switch (state.uiMode) {
    case MODE_NORMAL:
      break;
    case MODE_SET_ALARM:
      editingAlarmHour = !editingAlarmHour;
      break;
    case MODE_SET_TIMER:
      if (state.timerRunning) {
        state.timerRemainingMs = state.timerSecondsLeft * 1000UL;
        state.timerRunning = false;
      } else {
        if (state.timerSecondsLeft == 0) {
          state.timerSecondsLeft = TIMER_STEP_SECONDS;
        }
        state.timerRemainingMs = state.timerSecondsLeft * 1000UL;
        state.timerRunning = true;
        state.timerFinished = false;
        lastTimerUpdateAt = millis();
      }
      break;
    case MODE_ALERT_ACTIVE:
      dismissAlerts(state);
      break;
  }
}

void handleDoublePress(SystemState& state) {
  switch (state.uiMode) {
    case MODE_NORMAL:
      editingAlarmHour = true;
      state.uiMode = MODE_SET_ALARM;
      break;
    case MODE_SET_ALARM:
      state.uiMode = MODE_SET_TIMER;
      break;
    case MODE_SET_TIMER:
      state.uiMode = MODE_NORMAL;
      break;
    case MODE_ALERT_ACTIVE:
      dismissAlerts(state);
      break;
  }
}

void handleLongPress(SystemState& state) {
  switch (state.uiMode) {
    case MODE_NORMAL:
      setMotor(state, !state.motorRunning);
      break;
    case MODE_SET_ALARM:
      state.alarmEnabled = true;
      state.uiMode = MODE_NORMAL;
      break;
    case MODE_SET_TIMER:
      state.uiMode = MODE_NORMAL;
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
    } else if (state.uiMode == MODE_SET_TIMER) {
      int32_t seconds = static_cast<int32_t>(state.timerSecondsLeft) +
                        static_cast<int32_t>(steps) * TIMER_STEP_SECONDS;
      seconds = constrain(seconds, static_cast<int32_t>(TIMER_STEP_SECONDS),
                          static_cast<int32_t>(MAX_TIMER_SECONDS));
      state.timerSecondsLeft = static_cast<uint32_t>(seconds);
      if (!state.timerRunning) {
        state.timerRemainingMs = state.timerSecondsLeft * 1000UL;
      }
    }
  }

  const uint32_t now = millis();
  const uint8_t currentReading = digitalRead(ENCODER_PUSH_PIN);
  if (currentReading != rawButtonState) {
    rawButtonState = currentReading;
    buttonStateChangedAt = now;
  }

  if (rawButtonState != stableButtonState &&
      now - buttonStateChangedAt >= BUTTON_DEBOUNCE_MS) {
    stableButtonState = rawButtonState;
    if (stableButtonState == LOW) {
      buttonPressedAt = now;
      longPressHandled = false;
    } else if (!longPressHandled) {
      if (pendingClick && now - pendingClickAt <= DOUBLE_CLICK_MS) {
        pendingClick = false;
        handleDoublePress(state);
      } else {
        pendingClick = true;
        pendingClickAt = now;
      }
    }
  }

  if (stableButtonState == LOW && !longPressHandled &&
      now - buttonPressedAt >= LONG_PRESS_MS) {
    longPressHandled = true;
    pendingClick = false;
    handleLongPress(state);
  }

  if (pendingClick && now - pendingClickAt > DOUBLE_CLICK_MS) {
    pendingClick = false;
    handleShortPress(state);
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

  const float humidity = dht.readHumidity();
  const float temperature = dht.readTemperature();
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
    return;
  }

  state.dhtReady = true;
  state.temperatureC = temperature;
  state.humidityPercent = humidity;
  state.highTempAlert = temperature >= TEMP_HIGH_THRESHOLD;
  state.lowTempAlert = temperature <= TEMP_LOW_THRESHOLD;

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
  state.rtcReady = rtc.begin(&Wire1);
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
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), onEncoderTurn, FALLING);

  lastSensorReadAt = millis() - SENSOR_INTERVAL_MS;
  lastClockReadAt = millis() - CLOCK_INTERVAL_MS;
  lastTimerUpdateAt = millis();
}

void updateCore(SystemState& state) {
  const uint32_t now = millis();
  handleEncoder(state);
  readClock(state, now);
  readSensors(state, now);
  updateTimer(state, now);
  updateSnooze(state, now);
  updateServo(state, now);

  if (state.alarmRinging || state.timerFinished) {
    state.uiMode = MODE_ALERT_ACTIVE;
  } else if (state.uiMode == MODE_ALERT_ACTIVE) {
    state.uiMode = MODE_NORMAL;
  }
}
