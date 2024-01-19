#ifdef CORE_DEBUG_LEVEL
#undef CORE_DEBUG_LEVEL
#endif
#define CORE_DEBUG_LEVEL (1)

#include <stdint.h>
#include <stdbool.h>
#include <Arduino.h>
#include <unity.h>
#include "Actuator.cpp"
#include "Sensor.cpp"
#include "Shutter.cpp"

#define qSend(a, b, c) TEST_ASSERT_EQUAL(pdTRUE, xQueueSend(a, b, c));

void setup()
{
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);
    ESP_LOGI("main", "setup ENTRY");
    Wire.begin(I2C_SDA, I2C_SCL, I2C_FRQ);
    initShutters();
    initActuators();
    initSensors();
    ESP_LOGI("main", "setup EXITY");
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

bool waitForResult(uint8_t i, const char *desiredStateName, uint32_t abortTimeoutInMs)
{
    // just wait for some time for tasks to exchange messages between them
    std::string str1(desiredStateName);
    std::string str2(getShutter(i)->getCurrName());
    int64_t t = esp_timer_get_time();
    while(esp_timer_get_time() < t + (1000 *abortTimeoutInMs))
    {
        if (0 == str1.compare(str2))
        {
            return true;
        }
        delay(1);
    }
    return false;
}

void test_all(void)
{
    uint8_t i = 0;
    appMessage_t msg;

    // current state: idle, press down button, target state: down
    TEST_ASSERT_EQUAL_STRING("idle", getShutter(i)->getCurrName());
    msg.evt = SENSOR_TOUCH_EVT;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getDown();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    TEST_ASSERT_TRUE(waitForResult(i, "down", 200));

    // current state: down, press down button within debouncing time, target state: down
    TEST_ASSERT_EQUAL_STRING("down", getShutter(i)->getCurrName());
    msg.evt = SENSOR_TOUCH_EVT;
    msg.i2cAddr = getShutter(i)->getSensor().getI2cAddr();
    msg.data = getShutter(i)->getSensor().getDown();
    qSend(qHandleShutters, &msg, portMAX_DELAY);
    TEST_ASSERT_TRUE(waitForResult(i, "down", 20));

    // let state enter state: running
    TEST_ASSERT_TRUE(waitForResult(i, "running", HSM_DEBOUNCE_TIME + 10));
}

void loop()
{
    UNITY_BEGIN(); // IMPORTANT LINE!
    RUN_TEST(test_all);
    delay(500);
    UNITY_END(); // stop unit testing
}
