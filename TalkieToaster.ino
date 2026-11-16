#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>

#include "03_faith_tft_ui/tft_ui.h"

namespace {
RTC_DS3231 rtc;
bool rtcReady = false;
}

void setup() {
  SerialUSB.begin(115200);

  Wire1.begin();
  rtcReady = rtc.begin(&Wire1);
  if (!rtcReady) {
    SerialUSB.println("DS3231 not found; check 3V3, GND, SDA1, and SCL1");
  } else {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    SerialUSB.println("DS3231 set to firmware build time");
  }

  initTftUi();
}

void loop() {
  static uint32_t lastUpdateAt = 0;
  const uint32_t now = millis();
  if (now - lastUpdateAt < 1000) {
    return;
  }
  lastUpdateAt = now;

  if (!rtcReady) {
    updateTftUi(false, 0, 0, 0);
    return;
  }

  const DateTime current = rtc.now();
  updateTftUi(true, current.hour(), current.minute(), current.second());
}
