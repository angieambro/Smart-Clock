#pragma once

#include <Arduino.h>

constexpr uint8_t LED_PINS[] = {1, 2, 3, 4, 5, 6};
constexpr uint8_t DHT_PIN = 7;
constexpr uint8_t MOTOR_PIN = 9;

constexpr uint8_t TFT_RESET_PIN = 22;
constexpr uint8_t TFT_DC_PIN = 24;
constexpr uint8_t TFT_CS_PIN = 26;
constexpr uint8_t TFT_BACKLIGHT_PIN = 28;
constexpr uint8_t ENCODER_A_PIN = 30;
constexpr uint8_t ENCODER_B_PIN = 32;
constexpr uint8_t ENCODER_PUSH_PIN = 34;

// TFT uses hardware SPI on the Due ICSP header; RTC uses hardware I2C.
// DFPlayer uses Serial1 (TX1/D18, RX1/D19).