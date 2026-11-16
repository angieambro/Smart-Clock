#pragma once

#include <Arduino.h>

void initTftUi();
void updateTftUi(bool sensorReady, float temperatureC, float humidityPercent);
