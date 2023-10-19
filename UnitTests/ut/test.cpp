#include <Wire.h>
#include "gtest/gtest.h"
#include <Arduino.h>
#include <107-Arduino-CriticalSection.h>

unsigned long millis()
{
  return 0;
}

void pinMode(uint8_t pin, uint8_t mode)
{

}

void attachInterrupt(uint8_t pin, void *intRoutine, int mode)
{

}

void xQueueSend(QueueHandle_t handle, void* msg, uint32_t delay)
{

}

uint32_t xQueueReceive(QueueHandle_t handle, void* msg, uint32_t delay)
{
  return pdTRUE;
}
QueueHandle_t qHandleActuators = NULL;

QueueHandle_t xQueueCreate(uint32_t count, size_t size)
{
  return NULL;
}

void crit_sec_enter() {}


void crit_sec_leave() {}

TwoWire Wire = TwoWire(0);

TEST(TestCaseName, TestName) {


  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}