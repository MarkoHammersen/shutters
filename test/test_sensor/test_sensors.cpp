#if 0

#include <Arduino.h>
#include <unity.h>
#include "Sensor.cpp"

#define qSend(a,b,c) TEST_ASSERT_EQUAL(pdTRUE, xQueueSend(a, b, c));

QueueHandle_t qHandleShutters = NULL;

void tearDown(void)
{
  
}

void test_dummy(void)
{
  TEST_ASSERT_EQUAL(9, 10);
}


void setup()
{
  // NOTE!!! Wait for >2 secs
  // if board doesn't support software reset via Serial.DTR/RTS
  delay(2000);

  UNITY_BEGIN(); // IMPORTANT LINE!

  appMessage_t msg;
  qHandleShutters = xQueueCreate(32, sizeof(msg));
  TEST_ASSERT_NOT_NULL(qHandleShutters);

  initSensors();
  TEST_ASSERT_NOT_NULL(vTaskSensor);
  delay(1000); // wait for shutter queue being created.
}

void loop()
{
  delay(500);
  UNITY_END(); // stop unit testing
}

#endif