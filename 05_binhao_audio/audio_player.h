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
// Track configuration — matches the team's official track list
// (smartclock_dfplayer_track_list.txt). All MP3 files live in
// the /MP3 folder on the microSD card and are played with the
// DFPlayer "play MP3-folder file" command (0x0F):
//
//   0001  Alarm – electronic alarm sound        (SFX)
//   0002  Alarm Extreme – "Alarm!" x4           (looping alarm)
//   0003  Alarm Easy – "Good morning..."        (spare)
//   0004  Timer – egg timer ring
//   0005  High temperature
//   0006  Low temperature
//   0007  High humidity                         (no event yet — see note)
//   0008  Low humidity                          (no event yet — see note)
//   0009  Good Night                            (no event yet — see note)
//
// NOTE: SystemState currently has no humidity-alert or goodnight
// flags. Adding those events requires new shared fields — ask
// Angie & Feng before touching shared/system_state.h.
// ------------------------------------------------------------
constexpr uint8_t TRACK_ALARM      = 2;  // Alarm Extreme, loops while ringing
constexpr uint8_t TRACK_TIMER_DONE = 4;  // egg timer ring
constexpr uint8_t TRACK_HIGH_TEMP  = 5;
constexpr uint8_t TRACK_LOW_TEMP   = 6;

// Track 0002 lasts ~2.4 s. While alarmRinging stays true, the module
// re-fires the track slightly after it ends — software looping with
// zero dependence on DFPlayer reply parsing or the BUSY line.
constexpr uint32_t AUDIO_ALARM_RETRIGGER_MS = 2600;

// Master volume, 0 (silent) .. 30 (max)
constexpr uint8_t AUDIO_VOLUME = 22;

// Optional hardware BUSY pin (DFPlayer BUSY line is LOW while playing).
// Set to a real Due pin and wire DFPlayer BUSY -> that pin to enable,
// or leave 255 = feature disabled (not required: looping is software).
constexpr uint8_t AUDIO_BUSY_PIN = 255;

// Set to 1 to print module activity over SerialUSB (debug builds)
#ifndef AUDIO_DEBUG
  #define AUDIO_DEBUG 1
#endif

// --- Integration contract (do not change these signatures) ---
void initAudio();                            // called once from setup()
void updateAudio(const SystemState& state);  // called every loop iteration
