#include "core_logic.h"

SystemState testState;

void setup() {
  SerialUSB.begin(115200);
  initCore(testState);
}

void loop() {
  updateCore(testState);
}
