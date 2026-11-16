#include "tft_ui.h"

#include <Adafruit_ST7735.h>

#include "../shared/pins.h"

namespace {
Adafruit_ST7735 display(TFT_CS_PIN, TFT_DC_PIN, TFT_RESET_PIN);
}

void initTftUi() {
  pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BACKLIGHT_PIN, HIGH);

  display.initR(INITR_BLACKTAB);
  display.setRotation(1);
  display.fillScreen(ST77XX_BLACK);
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(0, 0);
  display.println("SMART CLOCK TEST");
  display.println();
  display.println("TFT: OK");
}

void updateTftUi(bool rtcReady, uint8_t hour, uint8_t minute, uint8_t second) {
  display.fillRect(0, 32, 160, 32, ST77XX_BLACK);
  display.setCursor(0, 32);
  display.setTextColor(rtcReady ? ST77XX_GREEN : ST77XX_RED);
  display.print("RTC: ");
  display.println(rtcReady ? "FOUND" : "NOT FOUND");

  if (rtcReady) {
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(0, 48);
    if (hour < 10) display.print('0');
    display.print(hour);
    display.print(':');
    if (minute < 10) display.print('0');
    display.print(minute);
    display.print(':');
    if (second < 10) display.print('0');
    display.print(second);
  }
}
