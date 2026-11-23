#include <Arduino.h>

#include "03_faith_tft_ui/tft_ui.h"
#include "01_angie_feng_master_core/core_logic.h"
#include "05_binhao_audio/audio_player.h"
#include "shared/pins.h"

namespace {
SystemState systemState;
UiState uiState;
}

void setup() {
  SerialUSB.begin(115200);
  initCore(systemState);
  initTftUi();
  initAudio();
}

void loop() {
  updateCore(systemState);
  updateAudio(systemState);

  uiState.rtcReady = systemState.rtcReady;
  uiState.hour = systemState.currentHour;
  uiState.minute = systemState.currentMinute;
  uiState.second = systemState.currentSecond;
  uiState.day = systemState.currentDay;
  uiState.month = systemState.currentMonth;
  uiState.year = systemState.currentYear;

  uiState.dhtReady = systemState.dhtReady;
  uiState.temperatureC = systemState.dhtReady ? systemState.temperatureC : 0.0f;
  uiState.humidityPercent = systemState.dhtReady ? systemState.humidityPercent : 0.0f;
  uiState.tempAlert = systemState.highTempAlert
    ? TempAlert::HIGH_TEMP
    : (systemState.lowTempAlert ? TempAlert::LOW_TEMP : TempAlert::NORMAL);

  uiState.mode = systemState.uiMode == MODE_SET_ALARM
    ? UiMode::ALARM_SETUP
    : (systemState.uiMode == MODE_SET_TIMER
      ? UiMode::TIMER_SETUP
      : (systemState.uiMode == MODE_HELP
        ? UiMode::HELP
        : UiMode::NORMAL));
  uiState.alarmEnabled = systemState.alarmEnabled;
  uiState.alarmHour = systemState.alarmHour;
  uiState.alarmMinute = systemState.alarmMinute;
  uiState.selectedAlarmField = systemState.alarmEditingHour
    ? AlarmField::HOUR
    : AlarmField::MINUTE;
  uiState.timerRunning = systemState.timerRunning;
  uiState.timerRemainingSec = systemState.timerSecondsLeft;
  uiState.selectedTimerField = systemState.timerEditingField;
  uiState.motorRunning = systemState.motorRunning;

  if (systemState.alarmRinging) {
    uiState.activeAlert = AlertType::RTC_ALARM;
  } else if (systemState.timerFinished) {
    uiState.activeAlert = AlertType::TIMER_DONE;
  } else if (systemState.highTempAlert) {
    uiState.activeAlert = AlertType::TEMP_HIGH;
  } else if (systemState.lowTempAlert) {
    uiState.activeAlert = AlertType::TEMP_LOW;
  } else {
    uiState.activeAlert = AlertType::NONE;
  }

  updateTftUi(uiState);
}
