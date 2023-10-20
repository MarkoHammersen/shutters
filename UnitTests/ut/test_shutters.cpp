#include "gtest/gtest.h"

#include <Arduino.h>
#include <arduino-timer.h>
#include "esp_log.h"
#include "Message.h"
#include "PinSetup.h"
#include "Window.h"
#include "hsm.hpp"
#include "Shutter.h"


class FooTest : public ::testing::Test {

  protected:
    FooTest() {
    }

    virtual ~FooTest() {
    }

    virtual void SetUp() {
      for (int i = 0; i < getSizeOfShutters(); i++)
      {
        getShutter(i)->startHsm();
        EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");
      }
    }

    virtual void TearDown() {
    }

  };

QueueHandle_t qHandleActuators = NULL;

unsigned long millis()
{
  return 0;
}

void xQueueSend(QueueHandle_t handle, void* msg, uint32_t delay)
{

}

uint32_t xQueueReceive(QueueHandle_t handle, void* msg, uint32_t delay)
{
  return pdTRUE;
}

QueueHandle_t xQueueCreate(uint32_t count, size_t size)
{
  return NULL;
}

TEST_F(FooTest, TestFullUpDownSequence) {
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");

    // press up button
    appMessage_t msg = { RUN, getShutter(i)->getSensor().getI2cAddr(), getShutter(i)->getSensor().getUp() };
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "up");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "running");

    // let run timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "stop");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");

    // press down
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getDown();
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "down");
  }
}

TEST_F(FooTest, TestFullDownUpSequence) {
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");

    // press down button

    appMessage_t msg = { RUN, getShutter(i)->getSensor().getI2cAddr(), getShutter(i)->getSensor().getDown() };
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "down");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "running");

    // let run timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "stop");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");

    // press up
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getUp();
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "up");
  }
}


TEST_F(FooTest, TestStopDownByDown) {
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");

    // press down button

    appMessage_t msg = { RUN, getShutter(i)->getSensor().getI2cAddr(), getShutter(i)->getSensor().getDown() };
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "down");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "running");

    // press a button
    msg.evt = RUN;
    msg.data = getShutter(i)->getSensor().getDown();
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "stop");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");
  }
}

TEST_F(FooTest, TestStopDownByUp) {
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");

    // press down button

    appMessage_t msg = { RUN, getShutter(i)->getSensor().getI2cAddr(), getShutter(i)->getSensor().getDown() };
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "down");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "running");

    // press a button
    msg.evt = RUN;
    msg.data = getShutter(i)->getSensor().getUp();
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "stop");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");
  }
}

TEST_F(FooTest, TestStopUpByUp) 
{
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");

    // press down button

    appMessage_t msg = { RUN, getShutter(i)->getSensor().getI2cAddr(), getShutter(i)->getSensor().getUp() };
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "up");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "running");

    // press a button
    msg.evt = RUN;
    msg.data = getShutter(i)->getSensor().getUp();
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "stop");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");
  }
}

TEST_F(FooTest, TestStopUpByDown) {
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");

    // press down button
    appMessage_t msg = { RUN, getShutter(i)->getSensor().getI2cAddr(), getShutter(i)->getSensor().getUp() };
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "up");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "running");

    // press a button
    msg.evt = RUN;
    msg.data = getShutter(i)->getSensor().getDown();
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "stop");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");
  }
}

TEST_F(FooTest, TestIgnoreButtonWhileDebouncing) {
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");

    // press button
    appMessage_t msg = { RUN, getShutter(i)->getSensor().getI2cAddr(), getShutter(i)->getSensor().getUp() };
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "up");

    // press a button while debouncing in up state is still in progress
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getUp();
  
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "up");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "running");

    // let run timer expire
    msg.evt = RUN;
    msg.data = getShutter(i)->getSensor().getDown();
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "stop");

    // press a button while debouncing in up state is still in progress
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getUp();
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "stop");

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    EXPECT_STREQ(getShutter(i)->getCurrName(), "idle");
  }
}