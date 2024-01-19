#if 0

#ifdef CORE_DEBUG_LEVEL
#undef CORE_DEBUG_LEVEL
#endif
#define CORE_DEBUG_LEVEL (1)

#include <Arduino.h>
#include <unity.h>
#include "shutter.cpp"

#define qSend(a, b, c) TEST_ASSERT_EQUAL(pdTRUE, xQueueSend(a, b, c));

QueueHandle_t qHandleActuators = NULL;

void test_dummy(void)
{
  TEST_ASSERT_EQUAL(9, 10);
}

static void checkResultActuatorUp(int i, const char *testName)
{
  appMessage_t msg;
  char buffer[8];
  sprintf(buffer, "%d", i);

  TEST_ASSERT_EQUAL_STRING_MESSAGE(getShutter(i)->getCurrName(), "up", buffer);

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT_MESSAGE(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY), buffer);
  TEST_ASSERT_EQUAL_MESSAGE(msg.i2cAddr, getShutter(i)->getActuator().getI2cAddr(), buffer);
  TEST_ASSERT_EQUAL_MESSAGE(msg.evt, STOP, buffer);

  TEST_ASSERT_EQUAL_MESSAGE(msg.data, getShutter(i)->getActuator().getDown(), buffer);

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT_MESSAGE(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY), buffer);
  TEST_ASSERT_EQUAL_MESSAGE(msg.i2cAddr, getShutter(i)->getActuator().getI2cAddr(), buffer);
  TEST_ASSERT_EQUAL_MESSAGE(msg.evt, RUN, buffer);
  TEST_ASSERT_EQUAL_MESSAGE(msg.data, getShutter(i)->getActuator().getUp(), buffer);
}

static void checkResultActuatorDown(int i, const char *testName)
{
  appMessage_t msg;

  TEST_ASSERT_EQUAL_STRING("down", getShutter(i)->getCurrName());

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT_MESSAGE(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY), testName);
  TEST_ASSERT_EQUAL_MESSAGE(getShutter(i)->getActuator().getI2cAddr(), msg.i2cAddr, testName);
  TEST_ASSERT_EQUAL_MESSAGE(STOP, msg.evt, testName);
  TEST_ASSERT_EQUAL_MESSAGE(getShutter(i)->getActuator().getUp(), msg.data, testName);

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT_MESSAGE(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY), testName);
  TEST_ASSERT_EQUAL_MESSAGE(getShutter(i)->getActuator().getI2cAddr(), msg.i2cAddr, testName);
  TEST_ASSERT_EQUAL_MESSAGE(RUN, msg.evt, testName);
  TEST_ASSERT_EQUAL_MESSAGE(getShutter(i)->getActuator().getDown(), msg.data, testName);
}

static void checkResultActuatorStop(int i)
{
  appMessage_t msg;

  TEST_ASSERT_EQUAL_STRING(getShutter(i)->getCurrName(), "stop");

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY));
  TEST_ASSERT_EQUAL(getShutter(i)->getActuator().getI2cAddr(), msg.i2cAddr);
  TEST_ASSERT_EQUAL(STOP, msg.evt);
  TEST_ASSERT_EQUAL(getShutter(i)->getActuator().getUp(), msg.data);

  memset(&msg, 0, sizeof(msg));
  TEST_ASSERT(pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY));
  TEST_ASSERT_EQUAL(getShutter(i)->getActuator().getI2cAddr(), msg.i2cAddr);
  TEST_ASSERT_EQUAL(STOP, msg.evt);
  TEST_ASSERT_EQUAL(getShutter(i)->getActuator().getDown(), msg.data);
}

static void forceStopByButton(int i, int runByBtn, int stopByBtn, const char *testName)
{
  appMessage_t msg;

  // press button
  msg.evt = RUN;
  msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
  msg.data = runByBtn;
  qSend(qHandleShutters, &msg, portMAX_DELAY);
  // check result
  if (runByBtn == getShutter(i)->getSensor().getUp())
  {
    checkResultActuatorUp(i, testName);
  }
  else
  {
    checkResultActuatorDown(i, testName);
  }

  // let debounce timer expire
  memset(&msg, 0, sizeof(msg));
  msg.evt = TIMEOUT;
  msg.data = i;
  qSend(qHandleShutters, &msg, portMAX_DELAY);
  // check result
  TEST_ASSERT_EQUAL_STRING_MESSAGE("running", getShutter(i)->getCurrName(), testName);

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
  TEST_ASSERT_EQUAL_STRING_MESSAGE("idle", getShutter(i)->getCurrName(), testName);

  while (pdTRUE == xQueueReceive(qHandleActuators, &msg, 10))
    ; // drain queue
}

void test_fullUpDownSequence(void)
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
    checkResultActuatorUp(i, __func__);

    // let debounce timer expire
    memset(&msg, 0, sizeof(msg));
    msg.evt = TIMEOUT;
    msg.data = i;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    TEST_ASSERT_EQUAL_STRING("running", getShutter(i)->getCurrName());

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
    TEST_ASSERT_EQUAL_STRING("idle", getShutter(i)->getCurrName());

    // press down button
    msg.evt = RUN;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getDown();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    checkResultActuatorDown(i, __func__);

    // let debounce timer expire
    memset(&msg, 0, sizeof(msg));
    msg.evt = TIMEOUT;
    msg.data = i;
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    // check result
    TEST_ASSERT_EQUAL_STRING("running", getShutter(i)->getCurrName());

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
    TEST_ASSERT_EQUAL_STRING("idle", getShutter(i)->getCurrName());
  }
}

void test_stopByButton(void)
{
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    appMessage_t msg;
    TEST_ASSERT_EQUAL_STRING("idle", getShutter(i)->getCurrName());

    forceStopByButton(i, getShutter(i)->getSensor().getUp(), getShutter(i)->getSensor().getDown(), __func__);
    forceStopByButton(i, getShutter(i)->getSensor().getUp(), getShutter(i)->getSensor().getUp(), __func__);
    forceStopByButton(i, getShutter(i)->getSensor().getDown(), getShutter(i)->getSensor().getDown(), __func__);
    forceStopByButton(i, getShutter(i)->getSensor().getDown(), getShutter(i)->getSensor().getUp(), __func__);
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
    checkResultActuatorUp(i, __func__);

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
    checkResultActuatorDown(i, __func__);

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

    while (pdTRUE == xQueueReceive(qHandleActuators, &msg, 10))
      ; // drain queue
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
    checkResultActuatorDown(i, __func__);

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

    while (pdTRUE == xQueueReceive(qHandleActuators, &msg, 10))
      ; // drain queue
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

void setUp(void)
{
   appMessage_t msg;

  while (pdTRUE == xQueueReceive(qHandleActuators, &msg, 10))
    ; // drain queue

  // let state machine come back to 'idle' state
  for (int i = 0; i < getSizeOfShutters(); i++)
  {
    // let debounce timer expire, if running
    while (1)
    {
      appMessage_t msg;
      memset(&msg, 0, sizeof(msg));
      msg.evt = TIMEOUT;
      msg.data = i;
      qSend(qHandleShutters, &msg, portMAX_DELAY);
      if (0 == strcmp(getShutter(i)->getCurrName(), "idle"))
      {
        break;
      }
      delay(10);
    }
  }
}

void tearDown(void)
{
  // same as setUp
  setUp();
}

void loop()
{
  // RUN_TEST(test_fullUpDownSequence);
  // RUN_TEST(test_stopByButton);
  RUN_TEST(test_ignoreButtonWhileUpDebouncing);
  // RUN_TEST(test_ignoreButtonWhileDownDebouncing);
  // RUN_TEST(test_ignoreButtonWhileStopDebouncing);
  delay(500);
  UNITY_END(); // stop unit testing
}
#endif