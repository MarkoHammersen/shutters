#include <Wire.h>
#include "gtest/gtest.h"

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

void noInterrupts()
{

}

void interrupts()
{

}

TwoWire Wire = TwoWire(0);

extern void setup();
extern void loop();

TEST(TestCaseName, TestName) {

  setup();
  loop();

  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}