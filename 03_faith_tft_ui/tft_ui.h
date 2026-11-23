/*
 * TFT Display UI Module (tft_ui.h / tft_ui.cpp)
 * Multi-screen interface for the 1.8" ST7735 (160x128) display on Arduino Due.
 *
 * Screens & Navigation:
 * - HOME (UiMode::NORMAL): Live DS3231 clock/date, DHT11 temp & humidity vertical bars,
 *   and top status icons (Alarm Bell, Countdown Timer, Fan).
 * - ALARM (UiMode::ALARM_SETUP): Editable HH:MM alarm time and ENABLED/DISABLED status.
 * - TIMER (UiMode::TIMER_SETUP): Editable HH:MM:SS countdown timer, max 24 hours.
 * - ALERTS (AlertType): Full-screen alerts for RTC_ALARM ("Good Morning!"),
 *   TIMER_DONE ("Time's Up!" + flashing 00:00), TEMP_HIGH ("Too Hot!"), and TEMP_LOW ("Too Cold!").
 *
 * Expected Rotary Encoder Controls:
 * - K0: Cycles bottom tabs (HOME -> ALARM -> TIMER).
 * - Home Mode: Hold toggles fan/motor.
 * - Alarm Setup: Turn adjusts selected field, Click switches Hour/Minute, Hold enables/disables alarm.
 * - Timer Setup: Push cycles HH/MM/SS; turn adjusts H/M by 1 and seconds by 10;
 *   Hold starts/stops timer.
 * - Active Alerts: Short Press dismisses any alert; Long Press snoozes RTC_ALARM for 5 mins
 *   (no snooze on timer or temperature alerts).
 *
 * Integration (main.cpp):
 * 1. Call initTftUi() once in setup().
 * 2. Update a UiState struct with live RTC, DHT11, button, and motor values in loop().
 * 3. Call updateTftUi(uiState) continuously at the end of loop() (do not wrap inside a 1s timer).
 */

#pragma once
#include <Arduino.h>
#include "../shared/system_state.h"

enum class UiMode : uint8_t {
  NORMAL = 0,
  ALARM_SETUP,
  TIMER_SETUP,
  HELP
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
  uint32_t timerRemainingSec = 0;
  TimerField selectedTimerField = TIMER_HOURS;

  // Motor (D9) & SG90 Servo (D8) State
  bool motorRunning = false;

  // Active Alert State (NONE when quiet)
  AlertType activeAlert = AlertType::NONE;
};

void initTftUi();
void updateTftUi(const UiState& state);
