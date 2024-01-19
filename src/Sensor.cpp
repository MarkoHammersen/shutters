#include <Arduino.h>
#include <arduino-timer.h>
#include <107-Arduino-CriticalSection.h>
#include <vector>
#include "Message.h"
#include "MCP23017.h"
#include "Sensor.h"

/* semaphore that will be used for reading of the rotary encoder */
static SemaphoreHandle_t iSRSemaphore = nullptr;
static TaskHandle_t xHandleSensors = NULL;
static Sensor sensorU18 = Sensor("U18", I2C_ADDR_SENSOR_U18, Wire);
static Sensor sensorU2 = Sensor("U2", I2C_ADDR_SENSOR_U2, Wire);

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
    sensorU2.interruptCallback();
}
static void sensorInterruptU2B(void)
{
    sensorU2.interruptCallback();
}
static void sensorInterruptU18A(void)
{
    sensorU18.interruptCallback();
}
static void sensorInterruptU18B(void)
{
    sensorU18.interruptCallback();
}

static bool sensorOnTimeoutU2(void *arg)
{
    MCP23017Port port = *(MCP23017Port *)arg;
    if (port == MCP23017Port::A)
    {
        attachInterrupt(digitalPinToInterrupt(SENSOR_U2_INTA), sensorInterruptU2A, FALLING);
    }
    else
    {
        attachInterrupt(digitalPinToInterrupt(SENSOR_U2_INTB), sensorInterruptU2B, FALLING);
    }
    return false; // do not repeat timer
}

static bool sensorOnTimeoutU18(void *arg)
{
    (void)xSemaphoreGive(iSRSemaphore);
    return false; // do not repeat timer
}

#if 1
static void vTaskSensor(void *arg)
{
    ESP_LOGI("Sensor", "task ENTRY");

    // initialize semaphore for reader task
    iSRSemaphore = xSemaphoreCreateBinary();

    sensorU2.init();
    pinMode(SENSOR_U2_INTA, INPUT);
    attachInterrupt(digitalPinToInterrupt(SENSOR_U2_INTA), sensorInterruptU2A, FALLING);
    pinMode(SENSOR_U2_INTB, INPUT);
    attachInterrupt(digitalPinToInterrupt(SENSOR_U2_INTB), sensorInterruptU2B, FALLING);

    sensorU18.init();
    pinMode(SENSOR_U18_INTA, INPUT);
    attachInterrupt(digitalPinToInterrupt(SENSOR_U18_INTA), sensorInterruptU18A, FALLING);
    pinMode(SENSOR_U18_INTB, INPUT);
    attachInterrupt(digitalPinToInterrupt(SENSOR_U18_INTB), sensorInterruptU18B, FALLING);

    while (1)
    {
        //if (xSemaphoreTake(iSRSemaphore, portMAX_DELAY) == pdPASS)
        {
             delay(10);
            if (sensorU2.isInterrupted())
            {
                log_i("U2 interrupt");
                if (sensorU2.getInterruptPort(MCP23017Port::A) > 0)
                {
                    sensorU2.startTimer(sensorOnTimeoutU2);
                    log_i("%s, A: %02x", sensorU2.getName(), sensorU2.getInterruptPort(MCP23017Port::A));
                }
                if (sensorU2.getInterruptPort(MCP23017Port::B) > 0)
                {
                    sensorU2.startTimer(sensorOnTimeoutU2);
                    log_i("%s, B: %02x", sensorU2.getName(), sensorU2.getInterruptPort(MCP23017Port::B));
                }
            }
            else
            {
                if ((LOW == digitalRead(SENSOR_U2_INTA)) || (LOW == digitalRead(SENSOR_U2_INTB)))
                {
                    log_i("U2 no interrupt, but INT line low");
                    sensorU2.clearInterrupts();
                    sensorU2.startTimer(sensorOnTimeoutU2);
                }
            }

            if (sensorU18.isInterrupted())
            {
                if (sensorU18.getInterruptPort(MCP23017Port::A) > 0)
                {
                    sensorU18.startTimer(sensorOnTimeoutU18);
                    log_i("%s, A: %02x", sensorU18.getName(), sensorU18.getInterruptPort(MCP23017Port::A));
                }
                if (sensorU18.getInterruptPort(MCP23017Port::B) > 0)
                {
                    sensorU18.startTimer(sensorOnTimeoutU18);
                    log_i("%s, B: %02x", sensorU18.getName(), sensorU18.getInterruptPort(MCP23017Port::B));
                }
            }
            else
            {
                if ((LOW == digitalRead(SENSOR_U18_INTA)) || (LOW == digitalRead(SENSOR_U18_INTB)))
                {
                    log_i("U18 no interrupt, but INT line low");
                    sensorU18.startTimer(sensorOnTimeoutU18);
                }
            }
            // for (Sensor s : sensors)
            // {
            //     for (uint8_t x = 0; x < 2; x++) // two ports
            //     {
            //         uint8_t port = s.getInterruptPort(static_cast<MCP23017Port>(x));
            //         for (uint8_t i = 0; i < 8; i++)
            //         {
            //             if ((port & (1u << i)) > 0)
            //             {
            //                 // interrupt on this pin
            //                 appMessage_t msg;
            //                 msg.i2cAddr = s.getI2cAddr();
            //                 msg.evt = SENSOR_TOUCH_EVT;
            //                 msg.data = i + (x * 8);
            //                 ESP_LOGI("sensors", "i2c: 0x%02x, evt: %d, data: 0x%2x", msg.i2cAddr, msg.evt, msg.data);
            //                 // xQueueSend(qHandleShutters, &msg, portMAX_DELAY);
            //             }
            //         }
            //     }
            // }
        }
    }
}
#else
static void vTaskSensor(void *arg)
{
    ESP_LOGI("Sensor", "task ENTRY");
    for (Sensor i : sensors)
    {
        i.init();
    }

    while (1)
    {
        for (Sensor s : sensors)
        {
            static uint16_t prevPorts = 0xFFFF; // all cleared
            uint16_t ports;
            ports = s.read();

            if (prevPorts != ports)
            {
                prevPorts = ports;
                log_i("%04x", ports);

                for (uint32_t i = 0; i < 16; i++)
                {
                    // let's be careful here:
                    // GPA7 & GPB7 Cannot Be Used as Inputs In MCP23017
                    // see: https://microchip.my.site.com/s/article/GPA7---GPB7-Cannot-Be-Used-as-Inputs-In-MCP23017
                    if (i == 7 || i == 15)
                    {
                        continue;
                    }

                    if ((ports & (1 << i)) == 0)
                    {
                        appMessage_t msg;
                        msg.i2cAddr = s.getI2cAddr();
                        msg.evt = SENSOR_TOUCH_EVT;
                        msg.data = i;
                        ESP_LOGI("sensors", "%s: i2c: 0x%02x, evt: %d, data: 0x%02x", s.getName(), msg.i2cAddr, msg.evt, msg.data);
                        // xQueueSend(qHandleShutters, &msg, portMAX_DELAY);
                    }
                }
            }
        }

        delay(50);
    }
}
#endif

void initSensors(void)
{
    xTaskCreate(vTaskSensor, "sensor", 4096, NULL, 10, &xHandleSensors);
    configASSERT(xHandleSensors != NULL);
}
