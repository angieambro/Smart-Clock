#include <Arduino.h>

#include "01_angie_feng_master_core/core_logic.h"

namespace {
SystemState state;
}

void setup() {
  SerialUSB.begin(115200);
  SerialUSB.println("Talkie Toaster starting");

  initCore(state);

  SerialUSB.println("Double-click: alarm/timer setup");
  SerialUSB.println("Hold: motor toggle, save alarm, exit timer, or snooze an alarm");
}

void loop() {
  updateCore(state);
}
