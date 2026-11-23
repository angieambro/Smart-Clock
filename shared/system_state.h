#pragma once

#include <Arduino.h>

enum CoreUiMode : uint8_t {
  MODE_NORMAL,
  MODE_SET_ALARM,
  MODE_SET_TIMER,
  MODE_HELP,
  MODE_ALERT_ACTIVE
};

enum TimerField : uint8_t {
  TIMER_HOURS,
  TIMER_MINUTES,
  TIMER_SECONDS
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
  bool highHumidityAlert = false;
  bool lowHumidityAlert = false;

  uint8_t alarmHour = 7;
  uint8_t alarmMinute = 0;
  bool alarmEditingHour = true;
  bool alarmEnabled = false;
  bool alarmRinging = false;
  uint32_t timerRemainingMs = 300000;
  uint32_t timerSecondsLeft = 300;
  TimerField timerEditingField = TIMER_HOURS;
  bool timerRunning = false;
  bool timerFinished = false;

  bool motorRunning = false;
  CoreUiMode uiMode = MODE_NORMAL;
};
