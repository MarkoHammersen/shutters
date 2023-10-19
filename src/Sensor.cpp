#include <Arduino.h>
#include <107-Arduino-CriticalSection.h>
#include <vector>
#include "Message.h"
#include "MCP23017.h"
#include "Sensor.h"

static TaskHandle_t xHandleSensors = NULL;
volatile bool interrupt = false;
static std::vector<Sensor> sensors = {
    Sensor(I2C_ADDR_SENSOR_U18),
    Sensor(I2C_ADDR_SENSOR_U35)};

static void sensorInterrupt()
{
    BaseType_t result = pdFAIL;
    BaseType_t switch_required = pdFALSE;

    result = xTaskNotifyFromISR(xHandleSensors, 0, eNoAction, &switch_required);
    if (result == pdPASS)
    {
        portYIELD_FROM_ISR(switch_required);
    }
}

static void vTaskSensor(void *arg)
{
    pinMode(SENSOR_U18_INTA, INPUT_PULLUP);
    pinMode(SENSOR_U18_INTB, INPUT_PULLUP);
    pinMode(SENSOR_U35_INTA, INPUT_PULLUP);
    pinMode(SENSOR_U35_INTB, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(SENSOR_U18_INTA), sensorInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(SENSOR_U18_INTB), sensorInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(SENSOR_U35_INTA), sensorInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(SENSOR_U35_INTB), sensorInterrupt, FALLING);

    for (Sensor i : sensors)
    {
        i.init();
    }

    while (1)
    {
        if (xTaskNotifyWait(0, 0, NULL, portMAX_DELAY) == pdTRUE)
        {
            {
                // enter critical section in constructor of "crit_sec", this is why we need the opening bracket above
                CriticalSection crit_sec;
                for (Sensor i : sensors)
                {
                    // find out which sensor ports have reported an interrupt
                    i.interruptedBy();
                }
                // leave critical section in de-constructor of "crit_sec", this is why we need the closing bracket below
            }

            for (Sensor s : sensors)
            {
                for (uint8_t x = 0; x < 2; x++) // two ports
                {
                    uint8_t port = s.getInterruptPort(static_cast<MCP23017Port>(x));
                    for (uint8_t i = 0; i < 8; i++)
                    {
                        if ((port & (1u << i)) > 0)
                        {
                            // interrupt on this pin
                            appMessage_t msg;
                            msg.i2cAddr = s.getI2cAddr();
                            msg.evt = TIMEOUT;
                            msg.data = i + (x * 8);
                            xQueueSend(qHandleShutters, &msg, portMAX_DELAY);
                        }
                    }
                }
                s.clearInterruptPorts();
            }
        }
    }
}

void initSensors(void)
{
    xTaskCreate(vTaskSensor, "sensor", 4096, NULL, 10, &xHandleSensors);
    configASSERT(xHandleSensors != NULL);
}