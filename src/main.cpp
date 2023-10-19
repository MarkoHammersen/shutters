#include <Arduino.h>
#include <Wire.h>

extern void initShutters();
extern void initActuators();  
extern void initSensors();

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
