#include "tft_ui.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "../shared/pins.h"

namespace {
  Adafruit_ST7735 display(TFT_CS_PIN, TFT_DC_PIN, TFT_RESET_PIN);

  // RGB565 Color Palette
  constexpr uint16_t C_BG         = 0x0841; // Dark slate
  constexpr uint16_t C_CARD       = 0x18E3; // Card dark grey
  constexpr uint16_t C_HEADER_NRM = 0x03EF; // Teal (Normal mode)
  constexpr uint16_t C_HEADER_SET = 0xFD20; // Orange (Setup modes)
  constexpr uint16_t C_ACCENT     = 0x07FF; // Cyan
  constexpr uint16_t C_WARN       = 0xF800; // Red
  constexpr uint16_t C_COLD       = 0x3DDF; // Light Blue
  constexpr uint16_t C_OK         = 0x07E0; // Green
  constexpr uint16_t C_MUTED      = 0x8410; // Grey

  UiState prev;
  bool firstDraw = true;

  // --- TINY DRAWING HELPERS ---

// Add a 0 before a number less than 10 (ex. 5 -> 05)
  void printTwoDigits(uint8_t val) {
    if (val < 10) {
      display.print('0');
    }
    display.print(val);
  }

// Draws a Bell icon to show an alarm has been set
  void drawBellIcon(int x, int y, bool active) {
    display.fillRect(x, y, 14, 14, C_BG);
    if (!active) return;

    display.fillCircle(x + 6, y + 5, 4, C_OK);
    display.fillRect(x + 2, y + 5, 9, 5, C_OK);
    display.fillRect(x + 1, y + 10, 11, 2, C_OK);
    display.fillCircle(x + 6, y + 12, 1, C_OK);
  }

// Draws a Sand Timer icon to show a timer has been set
  void drawSandTimerIcon(int x, int y, bool active) {
    display.fillRect(x, y, 12, 14, C_BG);
    if (!active) return;

    display.drawFastHLine(x + 1, y + 1, 9, C_ACCENT);
    display.drawFastHLine(x + 1, y + 12, 9, C_ACCENT);
    display.fillTriangle(x + 2, y + 2, x + 8, y + 2, x + 5, y + 6, C_ACCENT);
    display.drawTriangle(x + 2, y + 11, x + 8, y + 11, x + 5, y + 7, C_ACCENT);
  }

// Draws a Fan icon to indicate when the fan is on
  void drawFanIcon(int x, int y, bool active) {
    display.fillRect(x, y, 14, 14, C_BG);
    if (!active) return;

    display.fillCircle(x + 6, y + 2, 2, C_HEADER_SET);
    display.fillCircle(x + 6, y + 10, 2, C_HEADER_SET);
    display.fillCircle(x + 2, y + 6, 2, C_HEADER_SET);
    display.fillCircle(x + 10, y + 6, 2, C_HEADER_SET);
    display.fillCircle(x + 6, y + 6, 2, ST77XX_WHITE);
  }

// 
  void drawVerticalBar(int x, int y, int w, int h, float value, float minVal, float maxVal, uint16_t fillColor) {
    if (value < minVal) value = minVal;
    if (value > maxVal) value = maxVal;

    int innerH = h - 2;
    int fillH = (int)(((value - minVal) / (maxVal - minVal)) * innerH);

    display.drawRect(x, y, w, h, C_MUTED);
    if (innerH - fillH > 0) {
      display.fillRect(x + 1, y + 1, w - 2, innerH - fillH, C_BG);
    }
    if (fillH > 0) {
      display.fillRect(x + 1, y + 1 + (innerH - fillH), w - 2, fillH, fillColor);
    }
  }

// Draws the bar at the bottom of the screen indicating what mode the user is in/ letting them access a different mode
  void drawModeBar(UiMode mode) {
    uint16_t barColor;
    uint16_t textColor;

    if (mode == UiMode::NORMAL) {
      barColor  = C_HEADER_NRM;
      textColor = ST77XX_WHITE;
    } else {
      barColor  = C_HEADER_SET;
      textColor = ST77XX_BLACK;
    }

    display.fillRect(0, 110, 160, 18, barColor);
    display.setTextSize(1);
    display.setTextColor(textColor);
    display.setCursor(6, 115);

    switch (mode) {
      case UiMode::NORMAL:
        display.print("MONITOR  [2xClick:Setup]");
        break;
      case UiMode::ALARM_SETUP:
        display.print("SET ALARM [Hold:Enable]");
        break;
      case UiMode::TIMER_SETUP:
        display.print("SET TIMER  [Hold:Exit]");
        break;
    }
  }

  // --- SCREEN 1: NORMAL MONITOR MODE ---

  void drawNormalStatic() {
    display.fillScreen(C_BG);
    drawModeBar(UiMode::NORMAL);

    // Left card for Time & Date
    display.fillRoundRect(4, 18, 92, 88, 4, C_CARD);

    // Right card for Temp & Humidity bars
    display.fillRoundRect(100, 18, 56, 88, 4, C_CARD);
  }

  void renderNormal(const UiState& s, bool force) {
    if (force) {
      drawNormalStatic();
    }

    // Top Icons: Alarm Bell
    if (force || s.alarmEnabled != prev.alarmEnabled) {
      drawBellIcon(6, 2, s.alarmEnabled);
    }

    // Top Icons: Sandtimer + Remaining Time
    bool timerVisible = s.timerRunning || (s.timerRemainingSec > 0);
    bool prevTimerVisible = prev.timerRunning || (prev.timerRemainingSec > 0);

    if (force || timerVisible != prevTimerVisible || s.timerRemainingSec != prev.timerRemainingSec) {
      drawSandTimerIcon(30, 2, timerVisible);
      display.setTextSize(1);
      display.setCursor(45, 5);

      if (timerVisible) {
        if (s.timerRunning) {
          display.setTextColor(C_ACCENT, C_BG);
        } else {
          display.setTextColor(C_MUTED, C_BG);
        }
        printTwoDigits(s.timerRemainingSec / 60);
        display.print(':');
        printTwoDigits(s.timerRemainingSec % 60);
      } else {
        display.setTextColor(C_BG, C_BG);
        display.print("     ");
      }
    }

    // Top Icons: Fan
    if (force || s.motorRunning != prev.motorRunning) {
      drawFanIcon(140, 2, s.motorRunning);
    }

    // Left Card: Big Clock (HH:MM)
    if (force || s.rtcReady != prev.rtcReady || s.minute != prev.minute || s.hour != prev.hour) {
      display.fillRect(6, 32, 88, 28, C_CARD);
      display.setCursor(8, 34);
      if (!s.rtcReady) {
        display.setTextSize(2);
        display.setTextColor(C_WARN, C_CARD);
        display.print("NO RTC");
      } else {
        display.setTextSize(3);
        display.setTextColor(ST77XX_WHITE, C_CARD);
        printTwoDigits(s.hour);
        display.print(':');
        printTwoDigits(s.minute);
      }
    }

    // Left Card: Seconds & Date
    if (force || s.rtcReady != prev.rtcReady || s.second != prev.second || s.day != prev.day) {
      display.setTextSize(1);
      if (s.rtcReady) {
        display.setTextColor(C_ACCENT, C_CARD);
        display.setCursor(74, 22);
        display.print(':');
        printTwoDigits(s.second);

        display.setTextColor(C_MUTED, C_CARD);
        display.setCursor(16, 74);
        printTwoDigits(s.day);
        display.print('/');
        printTwoDigits(s.month);
        display.print('/');
        display.print(s.year);
      } else {
        display.setTextColor(C_CARD, C_CARD);
        display.setCursor(74, 22);
        display.print("   "); 
        display.setCursor(16, 74);
        display.print("          ");
      }
    }

    // Right Card: Temperature Vertical Bar & Value
    if (force || s.dhtReady != prev.dhtReady || s.tempAlert != prev.tempAlert ||
        abs(s.temperatureC - prev.temperatureC) >= 0.5f) {
      uint16_t tempColor;
      if (s.tempAlert == TempAlert::HIGH_TEMP) {
        tempColor = C_WARN;
      } else if (s.tempAlert == TempAlert::LOW_TEMP) {
        tempColor = C_COLD;
      } else {
        tempColor = C_OK;
      }

      float barTemp = 0.0f;
      if (s.dhtReady) {
        barTemp = s.temperatureC;
      }
      drawVerticalBar(108, 24, 14, 58, barTemp, 0.0f, 50.0f, tempColor);

      display.setTextSize(1);
      display.setCursor(102, 88);
      if (!s.dhtReady) {
        display.setTextColor(C_WARN, C_CARD);
        display.print("N/A ");
      } else {
        display.setTextColor(ST77XX_WHITE, C_CARD);
        display.print((int)round(s.temperatureC));
        display.print((char)247);
        display.print("C ");
      }
    }

    // Right Card: Humidity Vertical Bar & Value
    if (force || s.dhtReady != prev.dhtReady ||
        abs(s.humidityPercent - prev.humidityPercent) >= 1.0f) {
      float barHum = 0.0f;
      if (s.dhtReady) {
        barHum = s.humidityPercent;
      }
      drawVerticalBar(134, 24, 14, 58, barHum, 0.0f, 100.0f, C_ACCENT);

      display.setTextSize(1);
      display.setCursor(132, 88);
      if (!s.dhtReady) {
        display.setTextColor(C_WARN, C_CARD);
        display.print("N/A ");
      } else {
        display.setTextColor(C_ACCENT, C_CARD);
        display.print((int)round(s.humidityPercent));
        display.print("% ");
      }
    }
  }

  // --- SCREEN 2: ALARM SETUP MODE ---

  void renderAlarmSetup(const UiState& s, bool force) {
    if (force) {
      display.fillScreen(C_BG);
      drawModeBar(UiMode::ALARM_SETUP);
      display.fillRoundRect(10, 14, 140, 68, 6, C_CARD);

      display.setTextSize(1);
      display.setTextColor(C_MUTED);
      display.setCursor(10, 87);
      display.print("Click:Field  Turn:Adjust");
      display.setCursor(10, 98);
      display.print("Hold:Enable/Disable");
    }

    if (force || s.alarmHour != prev.alarmHour || s.alarmMinute != prev.alarmMinute ||
        s.selectedAlarmField != prev.selectedAlarmField || s.alarmEnabled != prev.alarmEnabled) {

      display.setTextSize(3);
      display.setCursor(35, 28);

      if (s.selectedAlarmField == AlarmField::HOUR) {
        display.setTextColor(ST77XX_BLACK, C_HEADER_SET);
      } else {
        display.setTextColor(ST77XX_WHITE, C_CARD);
      }
      printTwoDigits(s.alarmHour);

      display.setTextColor(ST77XX_WHITE, C_CARD);
      display.print(':');

      if (s.selectedAlarmField == AlarmField::MINUTE) {
        display.setTextColor(ST77XX_BLACK, C_HEADER_SET);
      } else {
        display.setTextColor(ST77XX_WHITE, C_CARD);
      }
      printTwoDigits(s.alarmMinute);

      display.setTextSize(1);
      display.setCursor(32, 62);
      if (s.alarmEnabled) {
        display.setTextColor(C_OK, C_CARD);
        display.print("STATUS: ENABLED ");
      } else {
        display.setTextColor(C_WARN, C_CARD);
        display.print("STATUS: DISABLED");
      }
    }
  }

  // --- SCREEN 3: TIMER SETUP MODE ---

  void renderTimerSetup(const UiState& s, bool force) {
    if (force) {
      display.fillScreen(C_BG);
      drawModeBar(UiMode::TIMER_SETUP);
      display.fillRoundRect(10, 14, 140, 68, 6, C_CARD);

      display.setTextSize(1);
      display.setTextColor(C_MUTED);
      display.setCursor(10, 87);
      display.print("Turn:+/-15s  Click:Run");
      display.setCursor(10, 98);
      display.print("Hold:Back to Monitor");
    }

    if (force || s.timerRemainingSec != prev.timerRemainingSec || s.timerRunning != prev.timerRunning) {
      display.setTextSize(3);
      display.setTextColor(C_ACCENT, C_CARD);
      display.setCursor(35, 28);
      printTwoDigits(s.timerRemainingSec / 60);
      display.print(':');
      printTwoDigits(s.timerRemainingSec % 60);
      display.setTextSize(1);
      display.setCursor(54, 62);
      if (s.timerRunning) {
      display.setTextColor(C_OK, C_CARD);
      display.print("RUNNING ");
      } else {
      display.setTextColor(C_HEADER_SET, C_CARD);
      display.print("PAUSED  ");
      }
    }
  }
  // --- SCREEN 4: ACTIVE / TRIGGERED ALERTS ---
  void renderTriggeredAlert(const UiState& s, bool force) {

  uint16_t bgColor   = C_WARN;
  uint16_t textColor = ST77XX_WHITE;

  if (s.activeAlert == AlertType::RTC_ALARM) {
    bgColor   = C_HEADER_NRM;
    textColor = ST77XX_WHITE;
  } else if (s.activeAlert == AlertType::TIMER_DONE) {
    bgColor   = C_BG;
    textColor = C_ACCENT;
  } else if (s.activeAlert == AlertType::TEMP_HIGH) {
    bgColor   = C_WARN;
    textColor = ST77XX_WHITE;
  } else if (s.activeAlert == AlertType::TEMP_LOW) {
    bgColor   = C_COLD;
    textColor = ST77XX_BLACK;
  }

  if (force) {
    display.fillScreen(bgColor);
    display.setTextColor(textColor, bgColor);
  if (s.activeAlert == AlertType::RTC_ALARM) {
    display.setTextSize(2);
  if (s.hour < 12) {
    display.setCursor(10, 22);
    display.print("Good Morning!");
  } else if (s.hour < 18) {
    display.setCursor(22, 16);
    display.print("Good");
    display.setCursor(22, 36);
    display.print("Afternoon!");
  } else {
    display.setCursor(22, 16);
    display.print("Good");
    display.setCursor(22, 36);
    display.print("Evening!");
  }
  } else if (s.activeAlert == AlertType::TIMER_DONE) {
    display.setTextSize(2);
    display.setCursor(22, 14);
    display.print("Time's Up!");
  } else if (s.activeAlert == AlertType::TEMP_HIGH) {
    display.setTextSize(2);
    display.setCursor(12, 12);
    display.print("Temperature");
    display.setCursor(12, 32);
    display.print("Alert:");
    display.setTextColor(ST77XX_YELLOW, bgColor);
    display.setCursor(12, 54);
    display.print("Too Hot!");
  } else if (s.activeAlert == AlertType::TEMP_LOW) {
    display.setTextSize(2);
    display.setCursor(12, 12);
    display.print("Temperature");
    display.setCursor(12, 32);
    display.print("Alert:");
    display.setTextColor(ST77XX_WHITE, bgColor);
    display.setCursor(12, 54);
    display.print("Too Cold!");
  }
  display.setTextSize(1);
  display.setTextColor(textColor, bgColor);
  display.setCursor(14, 92);
  display.print("Short Press : DISMISS");
  display.setCursor(14, 106);
  display.print("Long Press  : SNOOZE 5m");
  }
  // Non-blocking 500ms flash for 00:00 when timer finishes
  if (s.activeAlert == AlertType::TIMER_DONE) {
  bool flashOn = ((millis() / 500) % 2) == 0;
  display.setTextSize(3);
  display.setCursor(35, 44);
  if (flashOn) {
  display.setTextColor(C_WARN, bgColor);
  display.print("00:00");
  } else {
  display.setTextColor(bgColor, bgColor);
  display.print("     ");
  }
  }
  }
  }
  // --- PUBLIC FUNCTIONS (Called by your teammate's main code) ---
  void initTftUi() {
  pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BACKLIGHT_PIN, HIGH);
  display.initR(INITR_BLACKTAB);
  display.setRotation(1); // 160x128 landscape
  firstDraw = true;
  }


  void updateTftUi(const UiState& state) {
  // Check if we switched screens or if an alert just started/stopped
  bool viewChanged = firstDraw ||
  (state.activeAlert != prev.activeAlert) ||
  (state.activeAlert == AlertType::NONE && state.mode != prev.mode);
  // If any alert is active, show the alert screen; otherwise show the active mode
  if (state.activeAlert != AlertType::NONE) {
  renderTriggeredAlert(state, viewChanged);
  } else {
  switch (state.mode) {
  case UiMode::NORMAL:
  renderNormal(state, viewChanged);
  break;
  case UiMode::ALARM_SETUP:
  renderAlarmSetup(state, viewChanged);
  break;
  case UiMode::TIMER_SETUP:
  renderTimerSetup(state, viewChanged);
  break;
    }
  }
  
  // Save current state to our "sticky note" for the next loop
  prev = state;
  firstDraw = false;
}
