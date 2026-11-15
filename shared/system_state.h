#pragma once

#include <Arduino.h>

enum UiMode : uint8_t {
  MODE_NORMAL,
  MODE_SET_ALARM,
  MODE_SET_TIMER,
  MODE_ALERT_ACTIVE
};

struct SystemState {
  uint8_t currentHour = 0;
  uint8_t currentMinute = 0;
  uint8_t currentSecond = 0;
  uint16_t currentYear = 0;
  uint8_t currentMonth = 0;
  uint8_t currentDay = 0;
  bool rtcReady = false;

  float temperatureC = NAN;
  float humidityPercent = NAN;
  bool dhtReady = false;
  bool highTempAlert = false;
  bool lowTempAlert = false;

  uint8_t alarmHour = 7;
  uint8_t alarmMinute = 0;
  bool alarmEnabled = false;
  bool alarmRinging = false;
  uint32_t timerRemainingMs = 300000;
  uint32_t timerSecondsLeft = 300;
  bool timerRunning = false;
  bool timerFinished = false;

  bool motorRunning = false;
  UiMode uiMode = MODE_NORMAL;
};
