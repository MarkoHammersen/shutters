#include <Arduino.h>
#include <vector>
#include "Message.h"
#include "MCP23017.h"
#include "Actuator.h"

QueueHandle_t qHandleActuators = NULL;

static std::vector<Actuator> actuators = {
    Actuator(I2C_ADDR_ACTUATOR_U36, Wire),
    Actuator(I2C_ADDR_ACTUATOR_U37, Wire)
};

static void vTaskActuator(void *arg)
{
    appMessage_t msg;
    ESP_LOGI("Actuator", "task ENTRY");
    qHandleActuators = xQueueCreate(32, sizeof(msg));
    configASSERT(qHandleActuators != NULL);

    for (Actuator a : actuators)
    {
        a.init();
    }

    while (1)
    {
        memset(&msg, 0, sizeof(msg));
        if (pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY))
        {
            for (Actuator i : actuators)
            {
                i.processMsg(&msg);
            }
        }
    }
}

void initActuators(void)
{
    static TaskHandle_t xHandleActuators = NULL;
    xTaskCreate(vTaskActuator, "shutter", 4096, NULL, 10, &xHandleActuators);
    configASSERT(xHandleActuators != NULL);
}
