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
  display.println("DHT11 TEST");
  display.println();
  display.println("TFT: OK");
}

void updateTftUi(bool sensorReady, float temperatureC, float humidityPercent) {
  display.fillRect(0, 32, 160, 32, ST77XX_BLACK);
  display.setCursor(0, 32);
  if (!sensorReady) {
    display.setTextColor(ST77XX_RED);
    display.print("DHT11 READ FAILED");
    return;
  }

  display.setTextColor(ST77XX_WHITE);
  display.print("Temp: ");
  display.print(temperatureC, 1);
  display.println(" C");
  display.setCursor(0, 48);
  display.print("Humidity: ");
  display.print(humidityPercent, 1);
  display.print(" %");
}
