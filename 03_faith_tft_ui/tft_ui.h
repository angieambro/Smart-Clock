#pragma once
#include <Arduino.h>

enum class UiMode : uint8_t {
  NORMAL = 0,
  ALARM_SETUP,
  TIMER_SETUP
};

enum class AlarmField : uint8_t {
  HOUR = 0,
  MINUTE
};

enum class TempAlert : uint8_t {
  NORMAL = 0,
  HIGH_TEMP,
  LOW_TEMP
};

enum class AlertType : uint8_t {
  NONE = 0,
  RTC_ALARM,
  TIMER_DONE,
  TEMP_HIGH,
  TEMP_LOW
};

struct UiState {
  // System Mode
  UiMode mode = UiMode::NORMAL;

  // DS3231 RTC Data
  bool rtcReady = false;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
  uint8_t day = 1;
  uint8_t month = 1;
  uint16_t year = 2026;

  // DHT11 Sensor Data
  bool dhtReady = false;
  float temperatureC = 0.0f;
  float humidityPercent = 0.0f;
  TempAlert tempAlert = TempAlert::NORMAL;

  // Alarm State & Setup
  bool alarmEnabled = false;
  uint8_t alarmHour = 7;
  uint8_t alarmMinute = 0;
  AlarmField selectedAlarmField = AlarmField::HOUR;

  // Countdown Timer State & Setup
  bool timerRunning = false;
  uint16_t timerRemainingSec = 0;

  // Motor (D9) & SG90 Servo (D8) State
  bool motorRunning = false;

  // Active Alert State (NONE when quiet)
  AlertType activeAlert = AlertType::NONE;
};

void initTftUi();
void updateTftUi(const UiState& state);
