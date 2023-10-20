#include <Arduino.h>
#include <unity.h>
#include "shutter.cpp"

#define qSend(a,b,c) TEST_ASSERT_EQUAL(pdTRUE, xQueueSend(a, b, c));

QueueHandle_t qHandleActuators = NULL;


void setUp(void)
{
  // set stuff up here
}

void tearDown(void)
{
  // clean stuff up here
}

void test_dummy(void)
{
  TEST_ASSERT_EQUAL(9, 10);
}

static void checkResultActuatorUp(int i)
{
  appMessage_t msg;

  TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "up");

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY));
  TEST_ASSERT_EQUAL(msg.i2cAddr, getShutter(i)->getActuator().getI2cAddr());
  TEST_ASSERT_EQUAL(msg.evt, STOP);
  TEST_ASSERT_EQUAL(msg.data, getShutter(i)->getActuator().getDown());

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY));
  TEST_ASSERT_EQUAL(msg.i2cAddr, getShutter(i)->getActuator().getI2cAddr());
  TEST_ASSERT_EQUAL(msg.evt, RUN);
  TEST_ASSERT_EQUAL(msg.data, getShutter(i)->getActuator().getUp());
}

static void checkResultActuatorDown(int i)
{
  appMessage_t msg;

  TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "down");

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY));
  TEST_ASSERT_EQUAL(getShutter(i)->getActuator().getI2cAddr(), msg.i2cAddr);
  TEST_ASSERT_EQUAL(STOP, msg.evt);
  TEST_ASSERT_EQUAL(getShutter(i)->getActuator().getUp(), msg.data);

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY));
  TEST_ASSERT_EQUAL(getShutter(i)->getActuator().getI2cAddr(), msg.i2cAddr);
  TEST_ASSERT_EQUAL(RUN, msg.evt);
  TEST_ASSERT_EQUAL(getShutter(i)->getActuator().getDown(), msg.data );
}

static void checkResultActuatorStop(int i)
{
  appMessage_t msg;

  TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "stop");

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY));
  TEST_ASSERT_EQUAL(msg.i2cAddr, getShutter(i)->getActuator().getI2cAddr());
  TEST_ASSERT_EQUAL(msg.evt, STOP);
  TEST_ASSERT_EQUAL(msg.data, getShutter(i)->getActuator().getUp());

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY));
  TEST_ASSERT_EQUAL(msg.i2cAddr, getShutter(i)->getActuator().getI2cAddr());
  TEST_ASSERT_EQUAL(msg.evt, STOP);
  TEST_ASSERT_EQUAL(msg.data, getShutter(i)->getActuator().getDown());
}

static void forceStopByButton(int i, int runByBtn, int stopByBtn)
{
    appMessage_t msg;

    // press up button
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = runByBtn;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    if(runByBtn == getShutter(i)->getSensor().getUp())
    {
      checkResultActuatorUp(i);
    }
    else
    {
      checkResultActuatorDown(i);
    }    

    // let debounce timer expire
    memset(&msg, 0, sizeof(msg));
    msg.evt = TIMEOUT;
    msg.data = i;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "running");

    // press up button  to stop
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = stopByBtn;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    checkResultActuatorStop(i);

    // let debounce timer expire
    memset(&msg, 0, sizeof(msg));
    msg.evt = TIMEOUT;
    msg.data = i;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "idle");    

    while(pdTRUE == xQueueReceive(qHandleActuators, &msg, 10)); // drain queue
}

void test_fullUpDownSequence(void)
{
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    appMessage_t msg;
    TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "idle");

    // press up button
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getUp();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    checkResultActuatorUp(i);

    // let debounce timer expire
    memset(&msg, 0, sizeof(msg));
    msg.evt = TIMEOUT;
    msg.data = i;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "running");

    // let run timer expire
    memset(&msg, 0, sizeof(msg));
    msg.evt = TIMEOUT;
    msg.data = i;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    checkResultActuatorStop(i);

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "idle");

    // press down button
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getDown();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    checkResultActuatorDown(i);

    // let debounce timer expire
    memset(&msg, 0, sizeof(msg));
    msg.evt = TIMEOUT;
    msg.data = i;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "running");

    // let run timer expire
    memset(&msg, 0, sizeof(msg));
    msg.evt = TIMEOUT;
    msg.data = i;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    checkResultActuatorStop(i);

    // let debounce timer expire
    msg.evt = TIMEOUT;
    msg.data = i;
    getShutter(i)->processMsg(&msg);
    TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "idle");

    while(pdTRUE == xQueueReceive(qHandleActuators, &msg, 10)); // drain queue
  }
}

void test_stopByButton(void)
{
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    appMessage_t msg;
    TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "idle");

    forceStopByButton(i, getShutter(i)->getSensor().getUp(), getShutter(i)->getSensor().getDown());
    forceStopByButton(i, getShutter(i)->getSensor().getUp(), getShutter(i)->getSensor().getUp());
    forceStopByButton(i, getShutter(i)->getSensor().getDown(), getShutter(i)->getSensor().getDown());
    forceStopByButton(i, getShutter(i)->getSensor().getDown(), getShutter(i)->getSensor().getUp());
  }
}

void test_ignoreButtonWhileUpDebouncing(void)
{
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    appMessage_t msg;
    TEST_ASSERT_EQUAL_STRING("idle", getShutter(i)->getCurrName());

    // press up button
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getUp();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    checkResultActuatorUp(i);

    // press up button again
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getUp();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    memset(&msg, 0, sizeof(msg));
    TEST_ASSERT(pdFALSE == xQueueReceive(qHandleActuators, &msg, 20));
    TEST_ASSERT_EQUAL(msg.evt, NONE);
    TEST_ASSERT_EQUAL_STRING("up", getShutter(i)->getCurrName());

    // press down button again
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getDown();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    memset(&msg, 0, sizeof(msg));
    TEST_ASSERT(pdFALSE == xQueueReceive(qHandleActuators, &msg, 20));
    TEST_ASSERT_EQUAL(msg.evt, NONE);
    TEST_ASSERT_EQUAL_STRING("up", getShutter(i)->getCurrName());

    // let go back to idle
    msg.evt = TIMEOUT;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    TEST_ASSERT_EQUAL_STRING("idle", getShutter(i)->getCurrName());
    
    while(pdTRUE == xQueueReceive(qHandleActuators, &msg, 10)); // drain queue
  }
}

void test_ignoreButtonWhileDownDebouncing(void)
{
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    appMessage_t msg;
    TEST_ASSERT_EQUAL_STRING("idle", getShutter(i)->getCurrName());

    // press down button
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getDown();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    checkResultActuatorDown(i);

    // press down button again
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getDown();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    memset(&msg, 0, sizeof(msg));
    TEST_ASSERT(pdFALSE == xQueueReceive(qHandleActuators, &msg, 20));
    TEST_ASSERT_EQUAL(msg.evt, NONE);
    TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "down");

    // press up button 
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getUp();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    memset(&msg, 0, sizeof(msg));
    TEST_ASSERT(pdFALSE == xQueueReceive(qHandleActuators, &msg, 20));
    TEST_ASSERT_EQUAL(msg.evt, NONE);
    TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "down");

    // let go back to idle
    msg.evt = TIMEOUT;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    TEST_ASSERT_EQUAL_STRING("idle", getShutter(i)->getCurrName());

    while(pdTRUE == xQueueReceive(qHandleActuators, &msg, 10)); // drain queue
  }
}

void test_ignoreButtonWhileStopDebouncing(void)
{
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    appMessage_t msg;
    TEST_ASSERT_EQUAL_STRING("idle", getShutter(i)->getCurrName());

    // press down button
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getDown();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    checkResultActuatorDown(i);

    // let go to stop
    msg.evt = TIMEOUT;
    msg.data = i;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    delay(100);
    TEST_ASSERT_EQUAL_STRING("stop", getShutter(i)->getCurrName());

    // press down button
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getDown();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result, 
    TEST_ASSERT_EQUAL_STRING("stop", getShutter(i)->getCurrName());

    // press up button
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getUp();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result, 
    TEST_ASSERT_EQUAL_STRING("stop", getShutter(i)->getCurrName());

    // let go to idle
    msg.evt = TIMEOUT;
    msg.data = i;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    TEST_ASSERT_EQUAL_STRING("idle", getShutter(i)->getCurrName());

    while(pdTRUE == xQueueReceive(qHandleActuators, &msg, 10)); // drain queue
  }
}


void setup()
{
  // NOTE!!! Wait for >2 secs
  // if board doesn't support software reset via Serial.DTR/RTS
  delay(2000);

  UNITY_BEGIN(); // IMPORTANT LINE!

  appMessage_t msg;
  qHandleActuators = xQueueCreate(32, sizeof(msg));
  TEST_ASSERT_NOT_NULL(qHandleActuators);

  initShutters();
  TEST_ASSERT_NOT_NULL(vTaskShutter);
  delay(1000); // wait for shutter queue being created.
  TEST_ASSERT_NOT_NULL(qHandleShutters);
}

void loop()
{
  RUN_TEST(test_fullUpDownSequence);
  RUN_TEST(test_stopByButton);
  RUN_TEST(test_ignoreButtonWhileUpDebouncing);
  RUN_TEST(test_ignoreButtonWhileDownDebouncing);
  RUN_TEST(test_ignoreButtonWhileStopDebouncing);
  delay(500);
  UNITY_END(); // stop unit testing
}
