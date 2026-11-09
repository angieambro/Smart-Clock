#include <Adafruit_ST7735.h>
#include <Arduino.h>
#include <DHT.h>
#include <DFRobotDFPlayerMini.h>
#include <RTClib.h>
#include <SPI.h>
#include <Wire.h>

#include "pins.h"

namespace {
// Change these values here instead of searching through the rest of the sketch.
constexpr uint8_t DHT_TYPE = DHT11;
constexpr uint8_t STARTING_VOLUME = 10;
constexpr uint8_t MOTOR_PWM_DUTY = 200;
constexpr uint8_t MAX_VOLUME = 30;
constexpr uint32_t SENSOR_INTERVAL_MS = 2000;
constexpr uint32_t SCREEN_INTERVAL_MS = 1000;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint32_t LONG_PRESS_MS = 800;

DHT dht(DHT_PIN, DHT_TYPE);
RTC_DS3231 rtc;
DFRobotDFPlayerMini player;
Adafruit_ST7735 display(TFT_CS_PIN, TFT_DC_PIN, TFT_RESET_PIN);

bool rtcReady = false;
bool playerReady = false;
bool musicPlaying = false;
bool motorRunning = false;
uint8_t volume = STARTING_VOLUME;
float temperatureC = NAN;
float humidityPercent = NAN;

// The interrupt updates this number; loop() reads and clears it safely.
volatile int16_t encoderSteps = 0;
uint8_t rawButtonState = HIGH;
uint8_t stableButtonState = HIGH;
uint32_t buttonStateChangedAt = 0;
uint32_t buttonPressedAt = 0;
uint32_t lastSensorReadAt = 0;
uint32_t lastScreenUpdateAt = 0;

// This function runs briefly when encoder A falls from HIGH to LOW.
void onEncoderTurn() {
  encoderSteps += digitalRead(ENCODER_B_PIN) == HIGH ? 1 : -1;
}

void setMotor(bool shouldRun) {
  motorRunning = shouldRun;
  analogWrite(MOTOR_PIN, motorRunning ? MOTOR_PWM_DUTY : 0);
  SerialUSB.println(motorRunning ? "Motor on" : "Motor off");
}

void adjustVolume(int16_t change) {
  const int16_t nextVolume = static_cast<int16_t>(volume) + change;
  volume = static_cast<uint8_t>(constrain(nextVolume, 0, MAX_VOLUME));

  if (playerReady) {
    player.volume(volume);
  }
}

void handleEncoder() {
  // Pause interrupts briefly while copying the value that the interrupt changes.
  noInterrupts();
  const int16_t steps = encoderSteps;
  encoderSteps = 0;
  interrupts();

  if (steps != 0) {
    adjustVolume(steps);
    SerialUSB.print("Volume: ");
    SerialUSB.println(volume);
  }

  // A mechanical button can rapidly switch readings while its contacts settle.
  const uint8_t currentReading = digitalRead(ENCODER_PUSH_PIN);
  if (currentReading != rawButtonState) {
    rawButtonState = currentReading;
    buttonStateChangedAt = millis();
  }

  if (millis() - buttonStateChangedAt < BUTTON_DEBOUNCE_MS ||
      rawButtonState == stableButtonState) {
    return;
  }

  stableButtonState = rawButtonState;
  if (stableButtonState == LOW) {
    buttonPressedAt = millis();
    return;
  }

  const uint32_t pressDuration = millis() - buttonPressedAt;
  if (pressDuration >= LONG_PRESS_MS) {
    setMotor(!motorRunning);
  } else if (playerReady) {
    if (musicPlaying) {
      player.pause();
      musicPlaying = false;
    } else {
      player.play(1);
      musicPlaying = true;
    }
  }
}

void readSensors() {
  if (millis() - lastSensorReadAt < SENSOR_INTERVAL_MS) {
    return;
  }
  lastSensorReadAt = millis();

  humidityPercent = dht.readHumidity();
  temperatureC = dht.readTemperature();
  if (isnan(humidityPercent) || isnan(temperatureC)) {
    SerialUSB.println("DHT11 read failed; check its wiring");
    return;
  }

  SerialUSB.print("Temperature: ");
  SerialUSB.print(temperatureC, 1);
  SerialUSB.print(" C, humidity: ");
  SerialUSB.print(humidityPercent, 1);
  SerialUSB.println(" %");
}

void drawScreen() {
  display.fillScreen(ST7735_BLACK);
  display.setTextWrap(false);

  display.setTextColor(ST7735_CYAN);
  display.setTextSize(1);
  display.setCursor(4, 4);
  display.print("SMART CLOCK");

  display.setTextColor(ST7735_WHITE);
  display.setCursor(4, 20);
  if (rtcReady) {
    const DateTime currentTime = rtc.now();
    char timeText[9];
    char dateText[11];
    snprintf(timeText, sizeof(timeText), "%02u:%02u:%02u",
             currentTime.hour(), currentTime.minute(), currentTime.second());
    snprintf(dateText, sizeof(dateText), "%04u-%02u-%02u",
             currentTime.year(), currentTime.month(), currentTime.day());
    display.setTextSize(2);
    display.print(timeText);
    display.setTextSize(1);
    display.setCursor(4, 44);
    display.print(dateText);
  } else {
    display.print("RTC not found");
  }

  display.setCursor(4, 62);
  display.print("Temp: ");
  if (isnan(temperatureC)) {
    display.print("--.- C");
  } else {
    display.print(temperatureC, 1);
    display.print(" C");
  }

  display.setCursor(4, 76);
  display.print("Humidity: ");
  if (isnan(humidityPercent)) {
    display.print("-- %");
  } else {
    display.print(humidityPercent, 0);
    display.print(" %");
  }

  display.setCursor(4, 94);
  display.print("Audio: ");
  display.print(playerReady ? (musicPlaying ? "playing" : "paused") : "not found");
  display.setCursor(4, 108);
  display.print("Vol ");
  display.print(volume);
  display.print("   Motor: ");
  display.print(motorRunning ? "ON" : "OFF");
}

void updateScreen() {
  if (millis() - lastScreenUpdateAt < SCREEN_INTERVAL_MS) {
    return;
  }
  lastScreenUpdateAt = millis();
  drawScreen();
}
}

void setup() {
  // setup() runs once after power-up or reset.
  SerialUSB.begin(115200);
  SerialUSB.println("Smartclock starting");

  for (const uint8_t pin : LED_PINS) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  pinMode(MOTOR_PIN, OUTPUT);
  analogWriteResolution(8);
  analogWrite(MOTOR_PIN, 0);

  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(ENCODER_PUSH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), onEncoderTurn, FALLING);

  // The RTC is connected to the Due's built-in SDA and SCL I2C pins.
  Wire.begin();
  rtcReady = rtc.begin();
  if (!rtcReady) {
    SerialUSB.println("DS3231 not found; check SDA, SCL, and power");
  } else if (rtc.lostPower()) {
    // Set a new or reset RTC to the date and time when this firmware was built.
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    SerialUSB.println("RTC time set from firmware build time");
  }

  dht.begin();

  // On the Due, Serial1 is pins RX1/D19 and TX1/D18.
  Serial1.begin(9600);
  playerReady = player.begin(Serial1, true, true);
  if (playerReady) {
    player.volume(volume);
    SerialUSB.println("DFPlayer ready; put a track on its microSD card");
  } else {
    SerialUSB.println("DFPlayer not found; check Serial1 wiring and SD card");
  }

  // The TFT uses hardware SPI on the Due ICSP SCK and MOSI pins.
  display.initR(INITR_BLACKTAB);
  display.setRotation(1);

  // D28 has no hardware PWM on the Due, so this turns the backlight on steadily.
  pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BACKLIGHT_PIN, HIGH);

  SerialUSB.println("Turn encoder: volume; short press: play/pause");
  SerialUSB.println("Hold encoder for 0.8 seconds: motor on/off");
  SerialUSB.println("LED outputs are reserved and start off");
}

void loop() {
  // loop() repeats continuously; each task returns quickly so the others run too.
  handleEncoder();
  readSensors();
  updateScreen();
}