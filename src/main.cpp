#include <Arduino.h>
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

using namespace std;
#include "Actuator.h"
#include "Sensor.h"
#include "PortExpander.h"

using namespace std;
#define TAG "shutter"

void userInputA(void);
void userInputB(void);

void userInputA(void)
{
    int i= 1;
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(qHandleModel, &i, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void userInputB(void)
{
    int i= 0;
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(qHandleModel, &i, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}


class Shutter : public Hsm
{
protected:
  string room;
  string dir;
  Sensor sensor;
  Actuator actuator;

  State top;
  State idle;
  State stop;
  State running;
  State up;
  State down;

  uint32_t timeout;
  uint32_t tRunning;
  uint32_t tDebounce;
  uint32_t tStopDebounce;

public:
  Shutter(string room, string dir, Sensor s, Actuator a);
  Msg const *topHndlr(Msg const *msg);
  Msg const *idleHndlr(Msg const *msg);
  Msg const *stopHndlr(Msg const *msg);
  Msg const *runningHndlr(Msg const *msg);
  Msg const *upHndlr(Msg const *msg);
  Msg const *downHndlr(Msg const *msg);
};

static TaskHandle_t taskHandleModel = NULL;
static QueueHandle_t qHandleModel = NULL;

/*
Name	    Input				                    Output
          (MSP23017	Up	Down	Interrupt)	    (MSP23017	Up	Down)
Lina Ost	U36	        B0	B1	    B	            U4	        A6	A5
Lina Süd	U36	        B2	B3	    B	            U4	        A4	A3
Flur	    U36	        B4	B5	    B	            U4	        A2	A1
Wäsche	    U36	        B6	B7	    B	            U4	        A0	B6
Bad Süd	    U36	        A3	A2	    A	            U4	        B5	B4
Bad Nord	U36	        A1	A0	    A	            U4	        B3	B2
Julia West	U1	        B0	B1	    B	            U4	        B1	B0
Julia Ost	U1	        B2	B3	    B	            U37	        A6	A5
Paul Ost	U1	        B4	B5	    B	            U37	        A4	A3
Paul Nord	U1	        B6	B7	    B	            U37	        A2	A1
Büro	    U1	        A7	A6	    A	            U37	        B6	B5
*/
static vector<PortExpander> portExpanders = {
    {PORT_EXPANDER_TYPE::ACTUATOR, "U36", MCP23017(MCP23017_ACTUATOR_1_ADDR)},
    {PORT_EXPANDER_TYPE::ACTUATOR, "U37", MCP23017(MCP23017_ACTUATOR_2_ADDR)},
    {PORT_EXPANDER_TYPE::SENSOR, "U18", MCP23017(MCP23017_SENSOR_1_ADDR)},
    {PORT_EXPANDER_TYPE::SENSOR, "U35", MCP23017(MCP23017_SENSOR_2_ADDR)}
};

static vector<Shutter> shutters =
    {
        Shutter("Lina", "Ost", Sensor("U18", "B0", "B1"), Actuator("U36", "B0", "B1")),
        // Shutter("Lina", "Sued",     Sensor(Port("U36",  "B2"), Port("U36",  "B3")), Actuator(Port("U4",  "A4"), Port("A4", "A3"))),
        // Shutter("Flur", "Sued",     Sensor(Port("U36",  "B4"), Port("U36",  "B5")), Actuator(Port("U4",  "A2"), Port("A2", "A1"))),
        // Shutter("Waesche", "Sued",  Sensor(Port("U36",  "B6"), Port("U36",  "B7")), Actuator(Port("U4",  "A0"), Port("A0", "B6"))),
        // Shutter("Bad", "Sued",      Sensor(Port("U36",  "A3"), Port("U36",  "A2")), Actuator(Port("U4",  "A0"), Port("B5", "B4"))),
        // Shutter("Bad", "Nord",      Sensor(Port("U36",  "A1"), Port("U36",  "A0")), Actuator(Port("U4",  "B5"), Port("B3", "B2"))),
        // Shutter("Julia", "West",    Sensor(Port("U1",   "B0"), Port("U1",   "B1")), Actuator(Port("U4",  "B3"), Port("B1", "B0"))),
        // Shutter("Julia", "Nord",    Sensor(Port("U1",   "B2"), Port("U1",   "B3")), Actuator(Port("U37", "B1"), Port("A6", "A5"))),
        // Shutter("Paul", "Nord",     Sensor(Port("U1",   "B4"), Port("U1",   "B5")), Actuator(Port("U37", "B1"), Port("A4", "A3"))),
        // Shutter("Paul", "Ost",      Sensor(Port("U1",   "B6"), Port("U1",   "B7")), Actuator(Port("U37", "B1"), Port("A2", "A1"))),
        // Shutter("Buero", "Ost",     Sensor(Port("U1",   "A7"), Port("U1",   "A6")), Actuator(Port("U37", "B1"), Port("B6", "B5")))
};

enum ShutterEvents
{
  SHUTTER_UP_EVT,
  SHUTTER_DOWN_EVT,
  SHUTTER_IDLE_EVT,
  SHUTTER_TIMEOUT_EVT
};

const Msg shutterMsg[] = {
    (Event)SHUTTER_UP_EVT,
    (Event)SHUTTER_DOWN_EVT,
    (Event)SHUTTER_IDLE_EVT,
    (Event)SHUTTER_TIMEOUT_EVT};

Msg const *Shutter::downHndlr(Msg const *msg)
{
  switch (msg->evt)
  {
  case START_EVT:
    ESP_LOGI(TAG, "down-INIT\n");
    return 0;

  case ENTRY_EVT:
    ESP_LOGI(TAG, "down-ENTRY\n");
     return 0;

  case EXIT_EVT:
    ESP_LOGI(TAG, "down-EXIT\n");
    return 0;

  default:
     return 0;
  }
  return msg;
}

Msg const *Shutter::stopHndlr(Msg const *msg)
{
  switch (msg->evt)
  {
  case START_EVT:
    ESP_LOGI(TAG, "idle-INIT\n");
    return 0;

  case ENTRY_EVT:
    ESP_LOGI(TAG, "idle-ENTRY\n");
    return 0;

  case EXIT_EVT:
    ESP_LOGI(TAG, "idle-EXIT\n");
    return 0;

  }
  return msg;
}

Msg const *Shutter::idleHndlr(Msg const *msg)
{
  switch (msg->evt)
  {
  case START_EVT:
    ESP_LOGI(TAG, "idle-INIT\n");
    return 0;

  case ENTRY_EVT:
    ESP_LOGI(TAG, "idle-ENTRY\n");
    return 0;

  case EXIT_EVT:
    ESP_LOGI(TAG, "idle-EXIT\n");
    return 0;

   }
  return msg;
}

Msg const *Shutter::upHndlr(Msg const *msg)
{
  switch (msg->evt)
  {
  case START_EVT:
    ESP_LOGI(TAG, "up-INIT\n");
    return 0;

  case ENTRY_EVT:
    ESP_LOGI(TAG, "up-ENTRY\n");
    // relayModule->changeMode(relayDown, OFF);
    // relayModule->changeMode(relayUp, ON);
    tDebounce = 0u;
    return 0;

  case EXIT_EVT:
    ESP_LOGI(TAG, "up-EXIT\n");
    return 0;

  default:
    return 0;
  }
  return msg;
}

Msg const *Shutter::topHndlr(Msg const *msg)
{
  switch (msg->evt)
  {
  case START_EVT:
    ESP_LOGI(TAG, "top-INIT\n");
    return 0;
  case ENTRY_EVT:
    ESP_LOGI(TAG, "top-ENTRY\n");
    return 0;
  case EXIT_EVT:
    ESP_LOGI(TAG, "top-EXIT\n");
    return 0;
  }
  return msg;
}

Msg const *Shutter::runningHndlr(Msg const *msg)
{
  switch (msg->evt)
  {
  case START_EVT:
    ESP_LOGI(TAG, "running-INIT\n");
    return 0;
  case ENTRY_EVT:
    ESP_LOGI(TAG, "running-ENTRY\n");
    tRunning = 0u;
    return 0;
  case EXIT_EVT:
    ESP_LOGI(TAG, "running-EXIT\n");
    return 0;
  default:
    return 0;
  }
  return msg;
}

Shutter::Shutter(string room,
                 string dir,
                 Sensor sensor,
                 Actuator actuator)
    : Hsm("shutter", (EvtHndlr)&Shutter::topHndlr),
      idle("idle", &top, (EvtHndlr)&Shutter::idleHndlr),
      up("up", &up, (EvtHndlr)&Shutter::upHndlr),
      down("down", &down, (EvtHndlr)&Shutter::downHndlr),
      stop("stop", &stop, (EvtHndlr)&Shutter::stopHndlr),
      running("running", &running, (EvtHndlr)&Shutter::runningHndlr)
{
  room = room;
  dir = dir;
  sensor = sensor;
  actuator = actuator;
}

static void modelTask(void *args)
{
  (void *)args;
  uint8_t buffer[32];
  qHandleModel = xQueueCreate(128, sizeof(buffer));
  if (NULL == qHandleModel)
  {
    ESP_LOGE(TAG, "q creation failed");
    return;
  }

  while (xQueueReceive(qHandleModel, buffer, portMAX_DELAY))
  {
  }
}
void initializeModel()
{
  if (pdPASS != xTaskCreate(modelTask, "modelTask", 2 * 4096, NULL, 10, &taskHandleModel))
  {
    ESP_LOGE(TAG, "task setup failed");
    return;
  }
  ESP_LOGI(TAG, "task setup done!");
}

void setup()
{
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(1), userInputA, FALLING);
  attachInterrupt(digitalPinToInterrupt(2), userInputB, FALLING);
  attachInterrupt(digitalPinToInterrupt(1), userInputA, FALLING);
  attachInterrupt(digitalPinToInterrupt(2), userInputB, FALLING);  

  initializeModel();
}

void loop()
{
  // do nothing here, as software runs in dedicated tasks
}