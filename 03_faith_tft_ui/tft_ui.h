#pragma once

#include <Arduino.h>

void initTftUi();
void updateTftUi(bool rtcReady, uint8_t hour, uint8_t minute, uint8_t second);
