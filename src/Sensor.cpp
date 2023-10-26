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
    Sensor(I2C_ADDR_SENSOR_U35)
};

static void sensorInterrupt()
{
    BaseType_t xHigherPriorityTaskWoken;

    /* Clear the interrupt. */
    prvClearInterruptSource();

    /* xHigherPriorityTaskWoken must be initialised to pdFALSE.
    If calling vTaskNotifyGiveFromISR() unblocks the handling
    task, and the priority of the handling task is higher than
    the priority of the currently running task, then
    xHigherPriorityTaskWoken will be automatically set to pdTRUE. */
    xHigherPriorityTaskWoken = pdFALSE;

    /* Unblock the handling task so the task can perform
    any processing necessitated by the interrupt.  xHandlingTask
    is the task's handle, which was obtained when the task was
    created.  vTaskNotifyGiveFromISR() also increments
    the receiving task's notification value. */
    vTaskNotifyGiveFromISR(xHandleSensors, &xHigherPriorityTaskWoken);

    /* Force a context switch if xHigherPriorityTaskWoken is now
    set to pdTRUE. The macro used to do this is dependent on
    the port and may be called portEND_SWITCHING_ISR. */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
        /* Block to wait for a notification.  Here the RTOS
        task notification is being used as a counting semaphore.
        The task's notification value is incremented each time
        the ISR calls vTaskNotifyGiveFromISR(), and decremented
        each time the RTOS task calls ulTaskNotifyTake() - so in
        effect holds a count of the number of outstanding interrupts.
        The first parameter is set to pdFALSE, so the notification
        value is only decremented and not cleared to zero, and one
        deferred interrupt event is processed at a time.  See
        example 2 below for a more pragmatic approach. */
        (void)ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
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

void initSensors(void)
{
    xTaskCreate(vTaskSensor, "sensor", 4096, NULL, 10, &xHandleSensors);
    configASSERT(xHandleSensors != NULL);
}
