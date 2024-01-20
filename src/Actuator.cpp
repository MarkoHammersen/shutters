#include <Arduino.h>
#include <vector>
#include "Message.h"
#include "MCP23017.h"
#include "Actuator.h"

QueueHandle_t qHandleActuators = NULL;

static std::vector<Actuator> vActuators = {
    Actuator(I2C_ADDR_ACTUATOR_U36, Wire),
    Actuator(I2C_ADDR_ACTUATOR_U37, Wire)};

static void vTaskActuator(void *arg)
{
    appMessage_t msg;
    ESP_LOGI("Actuator", "task ENTRY");
    qHandleActuators = xQueueCreate(32, sizeof(msg));
    configASSERT(qHandleActuators != NULL);

    for (uint32_t i = 0; i < vActuators.size(); i++)
    {
        vActuators[i].init();
    }

    while (1)
    {
        memset(&msg, 0, sizeof(msg));
        if (pdTRUE == xQueueReceive(qHandleActuators, &msg, portMAX_DELAY))
        {
            log_i("%02d, %d, %d", msg.i2cAddr, msg.evt, msg.data);
            for (uint32_t i = 0; i < vActuators.size(); i++)
            {
                vActuators[i].processMsg(&msg);
            }
        }
    }
}

void initActuatorTask(void)
{
    static TaskHandle_t xHandleActuators = NULL;
    xTaskCreate(vTaskActuator, "actuator", 4096, NULL, 10, &xHandleActuators);
    configASSERT(xHandleActuators != NULL);
}
