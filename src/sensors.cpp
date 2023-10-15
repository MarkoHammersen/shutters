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
#include "sensors.h"

using namespace std;
#define TAG "sensors"

static TaskHandle_t taskHandleSensors = NULL;
static  QueueHandle_t qHandleSensors = NULL;

static void sensorsTask(void *args)
{
  (void *)args;
  uint8_t buffer[32];
  qHandleSensors = xQueueCreate(128, sizeof(buffer));
  if (NULL == qHandleSensors)
  {
    ESP_LOGE(TAG, "q creation failed");
    return;
  }

  while (xQueueReceive(qHandleSensors, buffer, portMAX_DELAY))
  {
  }
}
void initializeSensors()
{
  if (pdPASS != xTaskCreate(sensorsTask, "sensorsTask", 4096, NULL, 10, &taskHandleSensors))
  {
    ESP_LOGE(TAG, "task setup failed");
    return;
  }
  ESP_LOGI(TAG, "task setup done!");
}