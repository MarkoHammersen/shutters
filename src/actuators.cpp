#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "hsm.hpp"
#include "actuators.h"

using namespace std;
#define TAG "actuators"

static TaskHandle_t taskHandleActuators = NULL;
static  QueueHandle_t qHandleActuators = NULL;

static void actuatorsTask(void *args)
{
  (void *)args;
  uint8_t buffer[32];
  qHandleActuators = xQueueCreate(128, sizeof(buffer));
  if (NULL == qHandleActuators)
  {
    ESP_LOGE(TAG, "q creation failed");
    return;
  }

  while (xQueueReceive(qHandleActuators, buffer, portMAX_DELAY))
  {
  }
}
void initializeActuators()
{
  if (pdPASS != xTaskCreate(actuatorsTask, "actuatorsTask", 4096, NULL, 10, &taskHandleActuators))
  {
    ESP_LOGE(TAG, "task setup failed");
    return;
  }
  ESP_LOGI(TAG, "task setup done!");
}