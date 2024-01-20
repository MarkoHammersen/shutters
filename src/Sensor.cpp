#include <Arduino.h>
#include <arduino-timer.h>
#include <107-Arduino-CriticalSection.h>
#include <vector>
#include "Message.h"
#include "MCP23017.h"
#include "Sensor.h"

static SemaphoreHandle_t iSRSemaphore = nullptr;
static TaskHandle_t xHandleSensors = NULL;
static std::vector<Sensor> vSensors = {
    {Sensor("U2",
            I2C_ADDR_SENSOR_U2,
            SENSOR_U2_INTA,
            SENSOR_U2_INTB,
            Wire)},
    {Sensor("U18",
            I2C_ADDR_SENSOR_U18,
            SENSOR_U18_INTA,
            SENSOR_U18_INTB,
            Wire)}};

static void sensorInterrupt()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(iSRSemaphore, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

static void sensorInterruptU2A(void)
{
    sensorInterrupt();
}
static void sensorInterruptU2B(void)
{
    sensorInterrupt();
}
static void sensorInterruptU18A(void)
{
    sensorInterrupt();
}
static void sensorInterruptU18B(void)
{
    sensorInterrupt();
}

static void vTaskSensor(void *arg)
{
    ESP_LOGI("Sensor", "task ENTRY");

    // initialize semaphore for reader task
    iSRSemaphore = xSemaphoreCreateBinary();

    for (uint32_t i = 0; i < vSensors.size(); i++)
    {
        vSensors[i].init();
        pinMode(vSensors[i].getPinIntPort(MCP23017Port::A), INPUT);
        pinMode(vSensors[i].getPinIntPort(MCP23017Port::B), INPUT);
    }

    attachInterrupt(digitalPinToInterrupt(SENSOR_U2_INTA), sensorInterruptU2A, CHANGE);
    attachInterrupt(digitalPinToInterrupt(SENSOR_U2_INTB), sensorInterruptU2B, CHANGE);
    attachInterrupt(digitalPinToInterrupt(SENSOR_U18_INTA), sensorInterruptU18A, CHANGE);
    attachInterrupt(digitalPinToInterrupt(SENSOR_U18_INTB), sensorInterruptU18B, CHANGE);

    while (1)
    {
        if (xSemaphoreTake(iSRSemaphore, portMAX_DELAY) == pdPASS)
        {
            for (uint32_t i = 0; i < vSensors.size(); i++)
            {
                if ((LOW == digitalRead(vSensors[i].getPinIntPort(MCP23017Port::A))) || (LOW == digitalRead(vSensors[i].getPinIntPort(MCP23017Port::B))))
                {
                    uint16_t gpioAB;
                    if (true == vSensors[i].getGpioAB(&gpioAB))
                    {
                        log_i("%s interrupt: %04x", vSensors[i].getName(), gpioAB);
                        appMessage_t msg;
                        msg.i2cAddr = vSensors[i].getI2cAddr();
                        msg.evt = SENSOR_TOUCH_EVT;
                        msg.data = gpioAB;
                        xQueueSend(qHandleShutters, &msg, portMAX_DELAY);
                    }

                    delay(50);

                    // repeat giving the semaphore until digitalRead returns a high signal on the INT pins
                    xSemaphoreGive(iSRSemaphore);
                }
            }
        }
    }
}

void initSensorTask(void)
{
    xTaskCreate(vTaskSensor, "sensor", 4096, NULL, 10, &xHandleSensors);
    configASSERT(xHandleSensors != NULL);
}
