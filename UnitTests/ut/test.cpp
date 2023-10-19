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

void crit_sec_enter() {}
void crit_sec_leave() {}

TwoWire Wire = TwoWire(0);

extern void setup();
extern void loop();

TEST(TestCaseName, TestName) {

  setup();
  loop();

  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}