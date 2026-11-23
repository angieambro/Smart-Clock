#include "tft_ui.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "../shared/pins.h"

namespace {
  Adafruit_ST7735 display(TFT_CS_PIN, TFT_DC_PIN, TFT_RESET_PIN);

// RGB565 Color Palette
  constexpr uint16_t C_BG         = 0x0841; // Dark slate (Background features)
  constexpr uint16_t C_CARD       = 0x18E3; // Dark grey (Card Colour)
  constexpr uint16_t C_HEADER_NRM = 0x03EF; // Teal (Normal mode)
  constexpr uint16_t C_ORANGE     = 0xFF46; // Orange (Alarm)
  constexpr uint16_t C_CYAN       = 0x07FF; // Cyan (Humidity)
  constexpr uint16_t C_RED        = 0xF800; // Red (Temperature/Paused/ Disabled)
  constexpr uint16_t C_COLD       = 0x3DDF; // Light Blue
  constexpr uint16_t C_GREEN      = 0x07E0; // Green (Enabled/ Running)
  constexpr uint16_t C_MUTED      = 0x8410; // Grey (Font)
  constexpr uint16_t C_BLUE       = 0x861F; // Blue (Home)
  constexpr uint16_t C_PINK       = 0xFCDA; // Pink (Timer)

  UiState prev;
  bool firstDraw = true;


// --- Formatting / Small Icons ---

// Add a 0 before a number less than 10 (ex. 5 -> 05)
  void printTwoDigits(uint8_t val) {
    if (val < 10) {
      display.print('0');
    }
    display.print(val);
  }

  void printTimerHoursMinutes(uint32_t seconds) {
    const uint32_t totalMinutes = (seconds + 59UL) / 60UL;
    printTwoDigits(static_cast<uint8_t>(totalMinutes / 60UL));
    display.print(':');
    printTwoDigits(static_cast<uint8_t>(totalMinutes % 60UL));
  }

// Draws a bell icon to show an alarm has been set
  void drawBellIcon(int x, int y, bool active) {
    display.fillRect(x, y, 14, 14, C_BLUE);
    if (!active) return;

    display.fillCircle(x + 6, y + 5, 4, ST77XX_BLACK);
    display.fillRect(x + 2, y + 5, 9, 5, ST77XX_BLACK);
    display.fillRect(x + 1, y + 10, 11, 2, ST77XX_BLACK);
    display.fillCircle(x + 6, y + 12, 1, ST77XX_BLACK);
  }

// Draws a sand timer icon to show a timer has been set
  void drawSandTimerIcon(int x, int y, bool active) {
    display.fillRect(x, y, 12, 14, C_BLUE);
    if (!active) return;

    display.drawFastHLine(x + 1, y + 1, 9, ST77XX_BLACK);
    display.drawFastHLine(x + 1, y + 12, 9, ST77XX_BLACK);
    display.fillTriangle(x + 2, y + 2, x + 8, y + 2, x + 5, y + 6, ST77XX_BLACK);
    display.drawTriangle(x + 2, y + 11, x + 8, y + 11, x + 5, y + 7, ST77XX_BLACK);
  }

// Draws a fan icon to indicate when the fan is on
  void drawFanIcon(int x, int y, bool active) {
    display.fillRect(x, y, 14, 14, C_BLUE);
    if (!active) return;

    display.fillCircle(x + 6, y + 2, 2, ST77XX_BLACK);
    display.fillCircle(x + 6, y + 10, 2, ST77XX_BLACK);
    display.fillCircle(x + 2, y + 6, 2, ST77XX_BLACK);
    display.fillCircle(x + 10, y + 6, 2, ST77XX_BLACK);
    display.fillCircle(x + 6, y + 6, 2, ST77XX_WHITE);
  }

// Draws the vertical bars used for temperature and humidity
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

// Draws the 3-box navigation bar at the bottom: [ HOME ] [ ALARM ] [ TIMER ]
  void drawBottomTabs(UiMode mode) {
    display.drawFastHLine(0, 108, 160, C_MUTED);
    display.setTextSize(1);

    // HOME Tab (Blue when active)
    if (mode == UiMode::NORMAL) {
      display.fillRect(0, 109, 53, 19, C_BLUE);
      display.setTextColor(ST77XX_WHITE, C_BLUE);
    } else {
      display.fillRect(0, 109, 53, 19, C_CARD);
      display.setTextColor(C_MUTED, C_CARD);
    }
    display.setCursor(15, 115);
    display.print("HOME");

    display.drawFastVLine(53, 109, 19, C_BG);

    // ALARM Tab (Orange when active)
    if (mode == UiMode::ALARM_SETUP) {
      display.fillRect(54, 109, 53, 19, C_ORANGE);
      display.setTextColor(ST77XX_BLACK, C_ORANGE);
    } else {
      display.fillRect(54, 109, 53, 19, C_CARD);
      display.setTextColor(C_MUTED, C_CARD);
    }
    display.setCursor(66, 115);
    display.print("ALARM");

    display.drawFastVLine(107, 109, 19, C_BG);

    // TIMER Tab (Pink when active)
    if (mode == UiMode::TIMER_SETUP) {
      display.fillRect(108, 109, 52, 19, C_PINK);
      display.setTextColor(ST77XX_BLACK, C_PINK);
    } else {
      display.fillRect(108, 109, 52, 19, C_CARD);
      display.setTextColor(C_MUTED, C_CARD);
    }
    display.setCursor(120, 115);
    display.print("TIMER");
  }


  // --- NORMAL MONITOR MODE (HOME) ---

// NormalStatic for features that do not change with updates
  void drawNormalStatic() {
    display.fillScreen(C_BLUE);
    drawBottomTabs(UiMode::NORMAL);
    display.setTextSize(1);
    display.setTextColor(ST77XX_WHITE, C_BLUE);
    display.setCursor(95, 5);
    display.print("HOLD: HELP");
    display.setCursor(96, 5);
    display.print("HOLD: HELP");

    // Left card for Time & Date
    display.fillRoundRect(4, 18, 94, 88, 4, C_CARD);

    // Right card for Temp & Humidity bars
    display.fillRoundRect(102, 18, 54, 88, 4, C_CARD);
  }

// renderNormal for features that do change with updates
  void renderNormal(const UiState& s, bool force) {
    if (force) {
      drawNormalStatic();
    }

    // Top icon: Alarm Bell
    if (force || s.alarmEnabled != prev.alarmEnabled) {
      drawBellIcon(6, 2, s.alarmEnabled);
    }

    // Top icon: Sandtimer + Remaining Time
    bool timerVisible = s.timerRunning || (s.timerRemainingSec > 0);
    bool prevTimerVisible = prev.timerRunning || (prev.timerRemainingSec > 0);

    if (force || timerVisible != prevTimerVisible || s.timerRemainingSec != prev.timerRemainingSec) {
      drawSandTimerIcon(24, 2, timerVisible);
      display.setTextSize(1);
      display.setCursor(39, 5);

      if (timerVisible) {
        if (s.timerRunning) {
          display.setTextColor(ST77XX_BLACK, C_BLUE);
        } else {
          display.setTextColor(C_MUTED, C_BLUE);
        }
        printTimerHoursMinutes(s.timerRemainingSec);
      } else {
        display.setTextColor(C_BG, C_BG);
        display.print("     ");
      }
    }

    // Top icons: Fan
    if (force || s.motorRunning != prev.motorRunning) {
      drawFanIcon(77, 2, s.motorRunning);
    }

    // Left Card: Big Clock (HH:MM)
    if (force || s.rtcReady != prev.rtcReady || s.minute != prev.minute || s.hour != prev.hour) {
      display.fillRect(6, 20, 91, 60, C_CARD);
      display.setCursor(8, 22);
      display.setTextColor(C_MUTED);
      display.print("Current Time:");
      display.setCursor(8, 42);
      if (!s.rtcReady) {
        display.setTextSize(2);
        display.setTextColor(C_RED, C_CARD);
        display.print("NO RTC "); // If RTC not available
      } else {
        display.setTextSize(3);
        display.setTextColor(ST77XX_WHITE, C_CARD);
        printTwoDigits(s.hour);
        display.print(':');
        printTwoDigits(s.minute);
      }
    }

    // Left Card: Seconds & Date (DD/MM/YY)
    if (force || s.rtcReady != prev.rtcReady || s.second != prev.second || s.day != prev.day) {
      if (s.rtcReady) {
        // Seconds
        display.setTextSize(1);
        display.setTextColor(ST77XX_WHITE, C_CARD);
        display.setCursor(78, 72);
        display.print(':');
        printTwoDigits(s.second);

        // Date (Size 2: DD/MM/YY) underneath
        display.setTextSize(1);
        display.setTextColor(C_MUTED, C_CARD);
        display.setCursor(8, 94);
        display.print("Date:");
        printTwoDigits(s.day);
        display.print('/');
        printTwoDigits(s.month);
        display.print('/');
        printTwoDigits(s.year % 100);
      } else {
        display.fillRect(6, 68, 88, 36, C_CARD); // Erases seconds & date if RTC disconnects
      }
    }


    // Right card: Temperature vertical bar & value
    if (force || s.dhtReady != prev.dhtReady || s.tempAlert != prev.tempAlert ||
        abs(s.temperatureC - prev.temperatureC) >= 0.5f) {
      
      uint16_t tempColor;
      display.setTextSize(1);
      display.setCursor(115, 22);
      display.setTextColor(C_MUTED);
      display.print("T");

      if (s.tempAlert == TempAlert::HIGH_TEMP) {
        tempColor = C_RED;
      } else if (s.tempAlert == TempAlert::LOW_TEMP) {
        tempColor = C_COLD;
      } else {
        tempColor = C_GREEN;
      }

      float barTemp = 0.0f;
      if (s.dhtReady) {
        barTemp = s.temperatureC;
      }
      drawVerticalBar(110, 32, 14, 58, barTemp, 0.0f, 50.0f, tempColor);

      display.setTextSize(1);
      display.setCursor(106, 96);
      if (!s.dhtReady) {
        display.setTextColor(C_RED, C_CARD);
        display.print("N/A "); // If RTC not available
      } else {
        display.setTextColor(ST77XX_WHITE, C_CARD);
        display.print((int)round(s.temperatureC));
        display.print((char)247);
        display.print("C");
      }
    }

    // Right Card: Humidity Vertical Bar & Value
    if (force || s.dhtReady != prev.dhtReady ||
        abs(s.humidityPercent - prev.humidityPercent) >= 1.0f) {
      float barHum = 0.0f;

      display.setTextSize(1);
      display.setCursor(140, 22);
      display.setTextColor(C_MUTED);
      display.print("H");

      if (s.dhtReady) {
        barHum = s.humidityPercent;
      }
      drawVerticalBar(136, 32, 14, 58, barHum, 0.0f, 100.0f, C_CYAN);

      display.setTextSize(1);
      display.setCursor(136, 96);
      if (!s.dhtReady) {
        display.setTextColor(C_RED, C_CARD);
        display.print("N/A"); // If RTC not available
      } else {
        display.setTextColor(C_CYAN, C_CARD);
        display.print((int)round(s.humidityPercent));
        display.print("%");
      }
    }
  }

  // --- ALARM SETUP MODE ---

  void renderAlarmSetup(const UiState& s, bool force) {
    if (force) {
      display.fillScreen(C_ORANGE);
      drawBottomTabs(UiMode::ALARM_SETUP);
      display.fillRoundRect(4, 4, 152, 101, 6, C_CARD);

      display.setTextSize(1);
      display.setTextColor(C_MUTED, C_CARD);
      display.setCursor(35, 12);
      display.print("Hours : Minutes");
    }

    if (force || s.alarmHour != prev.alarmHour || s.alarmMinute != prev.alarmMinute ||
        s.selectedAlarmField != prev.selectedAlarmField || s.alarmEnabled != prev.alarmEnabled) {



      display.setTextSize(4);
      display.setCursor(20, 38);

      if (s.selectedAlarmField == AlarmField::HOUR) {
        display.setTextColor(ST77XX_WHITE, C_CARD);
      } else {
        display.setTextColor(C_ORANGE, C_CARD);
      }
      printTwoDigits(s.alarmHour);

      display.setTextColor(ST77XX_WHITE, C_CARD);
      display.print(':');

      if (s.selectedAlarmField == AlarmField::MINUTE) {
        display.setTextColor(ST77XX_WHITE, C_CARD);
      } else {
        display.setTextColor(C_ORANGE, C_CARD);
      }
      printTwoDigits(s.alarmMinute);

      display.setTextSize(1);
      display.setCursor(32, 83);
      if (s.alarmEnabled) {
        display.setTextColor(C_GREEN, C_CARD);
        display.print("STATUS: ENABLED ");
      } else {
        display.setTextColor(C_RED, C_CARD);
        display.print("STATUS: DISABLED");
      }
    }
  }

  // --- TIMER SETUP MODE ---

  void renderTimerSetup(const UiState& s, bool force) {
    if (force) {
      display.fillScreen(C_PINK);
      drawBottomTabs(UiMode::TIMER_SETUP);
      display.fillRoundRect(4, 4, 152, 101, 6, C_CARD);

      display.setTextSize(1);
      display.setTextColor(C_MUTED, C_CARD);
      display.setCursor(44, 12);
      display.print("HH : MM : SS");
    }

    if (force || s.timerRemainingSec != prev.timerRemainingSec ||
        s.timerRunning != prev.timerRunning ||
        s.selectedTimerField != prev.selectedTimerField) {
      const uint32_t hours = s.timerRemainingSec / 3600UL;
      const uint32_t minutes = (s.timerRemainingSec % 3600UL) / 60UL;
      const uint32_t seconds = s.timerRemainingSec % 60UL;
      display.setTextSize(3);
      display.setCursor(8, 39);
      display.setTextColor(
          s.selectedTimerField == TIMER_HOURS ? C_PINK : ST77XX_WHITE,
          C_CARD);
      printTwoDigits(static_cast<uint8_t>(hours));
      display.setTextColor(ST77XX_WHITE, C_CARD);
      display.print(':');
      display.setTextColor(
          s.selectedTimerField == TIMER_MINUTES ? C_PINK : ST77XX_WHITE,
          C_CARD);
      printTwoDigits(static_cast<uint8_t>(minutes));
      display.setTextColor(ST77XX_WHITE, C_CARD);
      display.print(':');
      display.setTextColor(
          s.selectedTimerField == TIMER_SECONDS ? C_PINK : ST77XX_WHITE,
          C_CARD);
      printTwoDigits(static_cast<uint8_t>(seconds));
      display.setTextSize(1);
      display.setCursor(56, 83);
      if (s.timerRunning) {
      display.setTextColor(C_GREEN, C_CARD);
      display.print("RUNNING ");
      } else {
      display.setTextColor(C_RED, C_CARD);
      display.print("PAUSED  ");
      }
    }
  }
  // --- ACTIVE / TRIGGERED ALERTS ---
  void renderTriggeredAlert(const UiState& s, bool force) {

  uint16_t bgColor   = C_RED;
  uint16_t textColor = ST77XX_WHITE;

  if (s.activeAlert == AlertType::RTC_ALARM) {
    bgColor   = C_HEADER_NRM;
    textColor = ST77XX_WHITE;
  } else if (s.activeAlert == AlertType::TIMER_DONE) {
    bgColor   = C_BG;
    textColor = C_PINK;
  } else if (s.activeAlert == AlertType::TEMP_HIGH) {
    bgColor   = C_RED;
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
    display.setCursor(22, 22);
    display.print("Good");
    display.setCursor(22, 42);
    display.print("Morning!");
  } else if (s.hour < 18) {
      display.setCursor(22, 22);
      display.print("Good");
      display.setCursor(22, 42);
      display.print("Afternoon!");
  } else {
      display.setCursor(22, 22);
      display.print("Good");
      display.setCursor(22, 42);
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

  if (s.activeAlert == AlertType::RTC_ALARM) {
    // Wake-up Alarm: Allow Dismiss or 5-min Snooze
    display.setCursor(14, 92);
    display.print("Short Press : DISMISS");
    display.setCursor(14, 106);
    display.print("Long Press  : SNOOZE 5m");
  } else {
    // Timer Done & Temperature Alerts: Dismiss only (no snooze)
    display.setCursor(14, 100);
    display.print("Short Press : DISMISS");
  }
}
  // Non-blocking 500ms flash for 00:00 when timer finishes
  if (s.activeAlert == AlertType::TIMER_DONE) {
    bool flashOn = ((millis() / 500) % 2) == 0;
    display.setTextSize(3);
    display.setCursor(35, 44);

  if (flashOn) {
    display.setTextColor(C_RED, bgColor);
    display.print("00:00");
  } else {
    display.setTextColor(bgColor, bgColor);
    display.print("     ");
  }
  }
}

void renderHelp(bool force) {
  if (!force) {
    return;
  }

  display.fillScreen(C_BG);
  display.setTextColor(ST77XX_WHITE, C_BG);
  display.setTextSize(2);
  display.setCursor(28, 4);
  display.print("CONTROLS");

  display.setTextSize(1);
  display.setTextColor(C_CYAN, C_BG);
  display.setCursor(5, 23);
  display.print("BOTTOM BUTTON");
  display.setTextColor(ST77XX_WHITE, C_BG);
  display.setCursor(5, 32);
  display.print("Short: change screen");
  display.setCursor(5, 41);
  display.print("Hold Home: fan on/off");
  display.setCursor(5, 50);
  display.print("Hold Alarm/Timer: reset");

  display.setTextColor(C_CYAN, C_BG);
  display.setCursor(5, 62);
  display.print("ROTARY ENCODER");
  display.setTextColor(ST77XX_WHITE, C_BG);
  display.setCursor(5, 71);
  display.print("Turn: adjust value");
  display.setCursor(5, 80);
  display.print("Push: next field/dismiss");
  display.setCursor(5, 89);
  display.print("Hold Alarm: toggle on/off");
  display.setCursor(5, 98);
  display.print("Hold Timer: start/stop");
  display.setCursor(5, 107);
  display.print("Hold Home: instructions");
  display.setTextColor(C_PINK, C_BG);
  display.setCursor(5, 118);
  display.print("Button/PUSH: Back to Home");
}

} // namespace

  void initTftUi() {
    pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
    digitalWrite(TFT_BACKLIGHT_PIN, HIGH);

    display.initR(INITR_BLACKTAB);
    display.setRotation(1);
    display.fillScreen(ST77XX_BLACK);
    firstDraw = true;
  }

  void updateTftUi(const UiState& state) {
    const bool force = firstDraw ||
                       state.mode != prev.mode ||
                       state.activeAlert != prev.activeAlert;

    if (state.activeAlert != AlertType::NONE) {
      renderTriggeredAlert(state, force);
    } else {
      switch (state.mode) {
        case UiMode::ALARM_SETUP:
          renderAlarmSetup(state, force);
          break;
        case UiMode::TIMER_SETUP:
          renderTimerSetup(state, force);
          break;
        case UiMode::HELP:
          renderHelp(force);
          break;
        case UiMode::NORMAL:
        default:
          renderNormal(state, force);
          break;
      }
    }

    prev = state;
    firstDraw = false;
  }
