#include "audio_player.h"

// ============================================================
// Binhao's audio module — DFPlayer Mini driver over Serial1
//
// Design rules (from TEAM_WORK.md / COPILOT_MASTER_SPEC.md):
//   * fully non-blocking: no delay(), no waiting for serial
//   * reacts only to SystemState changes (edge detection)
//   * alarm has the highest audio priority
//   * raw serial frames, so the master build needs no extra library
// ============================================================

namespace {

// --- DFPlayer Mini command bytes (datasheet) ---
constexpr uint8_t DF_CMD_RESET      = 0x0C;  // reset the module
constexpr uint8_t DF_CMD_MP3_FOLDER = 0x0F;  // play file N in /MP3 folder (playMp3Folder)
constexpr uint8_t DF_CMD_SET_VOLUME = 0x06;  // set volume 0..30
constexpr uint8_t DF_CMD_STOP       = 0x16;  // stop playback

// Frame layout:
//   0x7E 0xFF 0x06 CMD FEEDBACK DH DL CHK_HI CHK_LO 0xEF
constexpr uint8_t DF_FEEDBACK_NONE = 0x00;

// --- init state machine (the non-blocking replacement for delay()) ---
enum class InitStep : uint8_t {
  IDLE,         // initAudio() not called yet
  WAIT_RESET,   // reset sent, module is booting
  WAIT_VOLUME,  // volume command queued, give it time to land
  READY         // module usable
};

InitStep initStep      = InitStep::IDLE;
uint32_t stepStartedMs = 0;

constexpr uint32_t RESET_SETTLE_MS  = 3000;  // datasheet: ready ~2-3 s after reset
constexpr uint32_t VOLUME_SETTLE_MS = 300;

// --- command pacing: DFPlayer drops commands sent back-to-back ---
constexpr uint32_t MIN_CMD_GAP_MS = 80;
uint32_t lastCmdMs      = 0;
bool     cmdPending     = false;
uint8_t  pendingCmd     = 0;
uint16_t pendingArg     = 0;

bool    playerReady  = false;
bool    alarmLooping = false;   // true while the alarm track repeats
uint32_t alarmStartedMs = 0;    // when the current alarm playback began
uint8_t activeTrack  = 0;       // track currently requested (0 = silent)

// --- previous-cycle copies of the state fields we watch ---
bool prevAlarmRinging  = false;
bool prevTimerFinished = false;
bool prevHighTempAlert = false;
bool prevLowTempAlert  = false;

void sendCommand(uint8_t cmd, uint16_t arg) {
  // Checksum = negative sum of all bytes between 0xFF and DL (inclusive)
  const uint16_t sum      = (uint16_t)0xFF + 0x06 + cmd + DF_FEEDBACK_NONE
                          + (uint16_t)(arg >> 8) + (uint16_t)(arg & 0xFF);
  const uint16_t checksum = (uint16_t)(0x0000 - sum);

  const uint8_t frame[10] = {
    0x7E, 0xFF, 0x06, cmd, DF_FEEDBACK_NONE,
    (uint8_t)(arg >> 8), (uint8_t)(arg & 0xFF),
    (uint8_t)(checksum >> 8), (uint8_t)(checksum & 0xFF),
    0xEF
  };
  Serial1.write(frame, sizeof(frame));
  lastCmdMs = millis();

#if AUDIO_DEBUG
  SerialUSB.print("[audio] cmd=0x");
  SerialUSB.print(cmd, HEX);
  SerialUSB.print(" arg=");
  SerialUSB.println(arg);
#endif
}

// Queue a command; updateAudio() transmits it once the minimum gap
// since the previous command has passed. Never blocks.
void queueCommand(uint8_t cmd, uint16_t arg) {
  pendingCmd = cmd;
  pendingArg = arg;
  cmdPending = true;
}

void pumpPending() {
  if (cmdPending && (millis() - lastCmdMs) >= MIN_CMD_GAP_MS) {
    sendCommand(pendingCmd, pendingArg);
    cmdPending = false;
  }
}

void startTrack(uint8_t track) {
  if (!playerReady) {
#if AUDIO_DEBUG
    SerialUSB.println("[audio] not ready, request ignored");
#endif
    return;
  }
  queueCommand(DF_CMD_MP3_FOLDER, track);  // plays /MP3/000N.mp3
  activeTrack = track;
#if AUDIO_DEBUG
  SerialUSB.print("[audio] start /MP3/000");
  SerialUSB.print(track);
  SerialUSB.println(".mp3");
#endif
}

void stopPlayback() {
  queueCommand(DF_CMD_STOP, 0);
  activeTrack  = 0;
  alarmLooping = false;
#if AUDIO_DEBUG
  SerialUSB.println("[audio] stop");
#endif
}

}  // namespace

// ------------------------------------------------------------
// initAudio() — call once from setup()
// ------------------------------------------------------------
void initAudio() {
  Serial1.begin(9600);  // DFPlayer factory baud rate

  if (AUDIO_BUSY_PIN != 255) {
    pinMode(AUDIO_BUSY_PIN, INPUT);  // BUSY is LOW while a track plays
  }

  // Kick the module into a known state, then wait non-blockingly.
  sendCommand(DF_CMD_RESET, 0);
  initStep      = InitStep::WAIT_RESET;
  stepStartedMs = millis();
}

// ------------------------------------------------------------
// updateAudio() — call every iteration of loop()
// ------------------------------------------------------------
void updateAudio(const SystemState& state) {

  // --- 1. advance the init state machine ---
  if (initStep != InitStep::READY) {
    pumpPending();  // keep the pacing machine alive

    switch (initStep) {
      case InitStep::WAIT_RESET:
        if (millis() - stepStartedMs >= RESET_SETTLE_MS) {
          queueCommand(DF_CMD_SET_VOLUME, AUDIO_VOLUME);
          initStep      = InitStep::WAIT_VOLUME;
          stepStartedMs = millis();
        }
        break;

      case InitStep::WAIT_VOLUME:
        if (millis() - stepStartedMs >= VOLUME_SETTLE_MS) {
          playerReady = true;
          initStep    = InitStep::READY;
#if AUDIO_DEBUG
          SerialUSB.println("[audio] DFPlayer ready");
#endif
        }
        break;

      default:
        break;
    }
    return;  // nothing to react to until the player is ready
  }

  // --- 2. keep transmitting queued commands on schedule ---
  pumpPending();

  // --- 3. edge detection: act only on *changes*, not on levels ---
  const bool alarmOn    = state.alarmRinging    && !prevAlarmRinging;
  const bool alarmOff   = !state.alarmRinging   && prevAlarmRinging;
  const bool timerDone  = state.timerFinished   && !prevTimerFinished;
  const bool highTempOn = state.highTempAlert   && !prevHighTempAlert;
  const bool lowTempOn  = state.lowTempAlert    && !prevLowTempAlert;

  // --- 4. audio priority: the alarm always wins ---
  if (alarmOn) {
    startTrack(TRACK_ALARM);                    // fires the loop
    alarmStartedMs = millis();
    alarmLooping   = true;
  } else if (alarmOff && alarmLooping) {
    stopPlayback();                             // dismissed or snoozed
  } else if (alarmLooping && state.alarmRinging
             && (millis() - alarmStartedMs) >= AUDIO_ALARM_RETRIGGER_MS) {
    startTrack(TRACK_ALARM);                    // software loop: track ended,
    alarmStartedMs = millis();                  // ring again while still ringing
  } else if (!state.alarmRinging) {
    // One-shot events are only allowed when no alarm is ringing.
    if (timerDone) {
      startTrack(TRACK_TIMER_DONE);
    } else if (highTempOn) {
      startTrack(TRACK_HIGH_TEMP);
    } else if (lowTempOn) {
      startTrack(TRACK_LOW_TEMP);
    }
  }

  // --- 5. remember this cycle's state for the next edge comparison ---
  prevAlarmRinging  = state.alarmRinging;
  prevTimerFinished = state.timerFinished;
  prevHighTempAlert = state.highTempAlert;
  prevLowTempAlert  = state.lowTempAlert;
}
