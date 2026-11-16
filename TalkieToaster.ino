#include <Arduino.h>
#include <DHT.h>

#include "03_faith_tft_ui/tft_ui.h"
#include "shared/pins.h"

namespace {
constexpr uint8_t DHT_TYPE = DHT11;
constexpr uint32_t SENSOR_INTERVAL_MS = 2000;
DHT dht(DHT_PIN, DHT_TYPE);
uint32_t lastReadAt = 0;
}

void setup() {
  SerialUSB.begin(115200);
  dht.begin();
  initTftUi();
  lastReadAt = millis() - SENSOR_INTERVAL_MS;
}

void loop() {
  const uint32_t now = millis();
  if (now - lastReadAt < SENSOR_INTERVAL_MS) {
    return;
  }
  lastReadAt = now;

  const float humidity = dht.readHumidity();
  const float temperatureC = dht.readTemperature();
  const bool sensorReady = !isnan(humidity) && !isnan(temperatureC);
  updateTftUi(sensorReady, temperatureC, humidity);
}
