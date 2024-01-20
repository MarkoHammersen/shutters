
#include <Arduino.h>
#include <Wire.h>
#include "Message.h"
#include "MCP23017.h"

extern void initShutters();
extern void initActuatorTask();
extern void initSensorTask();

void setup()
{
  ESP_LOGI("main", "setup ENTRY");
  Wire.begin(I2C_SDA, I2C_SCL, I2C_FRQ);
  initActuatorTask();
  delay(20); // allow actuators some time to turn off all outputs
  // initShutters();
  // initSensorTask();
  ESP_LOGI("main", "setup EXITY");
}

void testActuators(void)
{
  static bool pressed = false;
  static bool on = false;
  if (digitalRead(0) == LOW)
  {
    appMessage_t msg;
    msg.i2cAddr = I2C_ADDR_ACTUATOR_U37;
    static int pin = 0;
    if (!pressed)
    {
      pressed = true;

      if (!on)
      {
        on = true;
        msg.evt = RUN;
        msg.data = pin;
        log_i("BOOT pin: %02x: pin = %d", msg.i2cAddr, msg.data);
        assert(pdTRUE == xQueueSend(qHandleActuators, &msg, portMAX_DELAY));
      }
      else
      {
        on = false;
        msg.evt = STOP;
        msg.data = pin++;
        log_i("BOOT pin: %02x: pin = %d", msg.i2cAddr, msg.data);
        assert(pdTRUE == xQueueSend(qHandleActuators, &msg, portMAX_DELAY));
      }
    }
  }
  else
  {
    pressed = false;
  }
}

void loop()
{
#if 0 // for testing actuators
testActuators();
#endif
}
