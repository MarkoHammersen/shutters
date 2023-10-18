#include "gtest/gtest.h"

unsigned long millis()
{
  return 0;
}

void pinMode(uint8_t pin, uint8_t mode)
{

}

TEST(TestCaseName, TestName) {
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}