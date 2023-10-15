#include <Arduino.h>
#include "model.h"
#include "actuators.h"
#include "sensors.h"

void setup() {
  // put your setup code here, to run once:
  initializeModel();
  initializeActuators();
  initializeSensors();
}

void loop() {
  // do nothing here, as software runs in dedicated tasks
}