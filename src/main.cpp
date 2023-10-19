#include <Arduino.h>
#include <107-Arduino-CriticalSection.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "esp_log.h"
#include "hsm.hpp"
#include "Message.h"
#include "Actuator.h"
#include "Sensor.h"
#include "Shutter.h"

void setup()
{
  Wire.begin(I2C_SDA, I2C_SCL, I2C_FRQ);
  initShutters();
  initActuators();  
  initSensors();
}

void loop()
{
  
}
