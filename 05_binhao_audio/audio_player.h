#pragma once

#include <Arduino.h>

// Works both inside the repo layout (shared/system_state.h) and
// inside the standalone test project (system_state.h next to this file).
#if __has_include("../shared/system_state.h")
  #include "../shared/system_state.h"
#else
  #include "system_state.h"
#endif

// ------------------------------------------------------------
// Track configuration — change these numbers if the SD card
// layout changes. Track N = the Nth MP3 in the SD card root
// (files named 0001xxx.mp3, 0002xxx.mp3, ...).
// ------------------------------------------------------------
constexpr uint8_t TRACK_ALARM      = 1;  // 0001_ALARM.mp3
constexpr uint8_t TRACK_TIMER_DONE = 2;  // 0002_TIMER_DONE.mp3
constexpr uint8_t TRACK_HIGH_TEMP  = 3;  // 0003_HIGH_TEMP.mp3
constexpr uint8_t TRACK_LOW_TEMP   = 4;  // 0004_LOW_TEMP.mp3

// Master volume, 0 (silent) .. 30 (max)
constexpr uint8_t AUDIO_VOLUME = 22;

// Optional hardware BUSY pin (DFPlayer BUSY line is LOW while playing).
// Set to a real Due pin and wire DFPlayer BUSY -> that pin to enable,
// or leave 255 = feature disabled (looping is handled by command 0x08,
// so BUSY is a nice-to-have, not a requirement).
constexpr uint8_t AUDIO_BUSY_PIN = 255;

// Set to 1 to print module activity over SerialUSB (debug builds)
#ifndef AUDIO_DEBUG
  #define AUDIO_DEBUG 1
#endif

// --- Integration contract (do not change these signatures) ---
void initAudio();                            // called once from setup()
void updateAudio(const SystemState& state);  // called every loop iteration
