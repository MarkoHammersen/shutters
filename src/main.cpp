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
enum class MCP23017Name : uint8_t
{
  U18,
  U35,
  U36,
  U37
};

#include "Actuator.h"
#include "Sensor.h"

using namespace std;
#define TAG "shutter"

typedef enum
{
  SENSOR_INT_U18_A,
  SENSOR_INT_U18_B,
  SENSOR_INT_U35_A,
  SENSOR_INT_U35_B
} sensorEvent_t;

typedef enum HsmEvents
{
  SHUTTER_UP_EVT,
  SHUTTER_DOWN_EVT,
  SHUTTER_IDLE_EVT,
  SHUTTER_TIMEOUT_EVT
} hsmEvent_t;

static void sendMsgFromInt(sensorEvent_t &msg);
static void sensorIntU18A();
static void sensorIntU18B();
static void sensorIntU35A();
static void sensorIntU35B();

class Shutter : public Hsm
{
protected:
  string room;
  string dir;

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
  Sensor sensor;
  Actuator actuator;
  Shutter(string room, string dir, Sensor s, Actuator a);
  Msg const *topHndlr(Msg const *msg);
  Msg const *idleHndlr(Msg const *msg);
  Msg const *stopHndlr(Msg const *msg);
  Msg const *runningHndlr(Msg const *msg);
  Msg const *upHndlr(Msg const *msg);
  Msg const *downHndlr(Msg const *msg);
};

static TaskHandle_t taskHandleModel = NULL;
static TaskHandle_t taskHandleSensor = NULL;
static TaskHandle_t taskHandleActuator = NULL;
static QueueHandle_t qHandleModel = NULL;
static QueueHandle_t qHandleSensor = NULL;
static QueueHandle_t qHandleActuator = NULL;

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

static bool interrupt = false;
static MCP23017 mcpU36 = MCP23017(MCP23017_U36_ADDR);
static MCP23017 mcpU18 = MCP23017(MCP23017_U18_ADDR);

static vector<Shutter> shutters =
    {
        Shutter("Lina", "Ost", Sensor(MCP23017Name::U18, MCP23017Pin::GPB0, MCP23017Pin::GPB1), Actuator(&mcpU36, "B0", "B1")),
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

const Msg shutterMsg[] = {
    (Event)SHUTTER_UP_EVT,
    (Event)SHUTTER_DOWN_EVT,
    (Event)SHUTTER_IDLE_EVT,
    (Event)SHUTTER_TIMEOUT_EVT};

static void sendMsgFromInt(sensorEvent_t &msg)
{
  BaseType_t xHigherPriorityTaskWoken;
  xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(qHandleModel, &msg, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken)
  {
    portYIELD_FROM_ISR();
  }
}

static void sensorIntU18A()
{
  // sensorEvent_t msg = SENSOR_INT_U18_A;
  // sendMsgFromInt(msg);
  interrupt = true;
}

static void sensorIntU18B()
{
  // sensorEvent_t msg = SENSOR_INT_U18_B;
  // sendMsgFromInt(msg);
  interrupt = true;
}

static void sensorIntU35A()
{
  // sensorEvent_t msg = SENSOR_INT_U35_A;
  // sendMsgFromInt(msg);
  interrupt = true;
}

static void sensorIntU35B()
{
  // sensorEvent_t msg = SENSOR_INT_U35_B;
  // sendMsgFromInt(msg);
  interrupt = true;
}

Msg const *Shutter::downHndlr(Msg const *msg)
{
  switch (msg->evt)
  {
  case START_EVT:
    ESP_LOGI(TAG, "down-INIT\n");
    return 0;

  case ENTRY_EVT:
    ESP_LOGI(TAG, "down-ENTRY\n");
    actuator.switchOff(actuator.getUp());
    actuator.switchOn(actuator.getDown());
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
    actuator.switchOff(actuator.getDown());
    actuator.switchOff(actuator.getUp());
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
    actuator.switchOff(actuator.getDown());
    actuator.switchOn(actuator.getUp());
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

static void sensorTask(void *args)
{
  (void *)args;
  sensorEvent_t msg;
  qHandleSensor = xQueueCreate(128, sizeof(msg));
  if (NULL == qHandleSensor)
  {
    ESP_LOGE(TAG, "sesnor q creation failed");
    return;
  }

  memset(&msg, 0, sizeof(msg));
  while (xQueueReceive(qHandleSensor, &msg, portMAX_DELAY))
  {
    uint8_t port = 0;
    uint8_t portA = 0;
    uint8_t portB = 0;
    switch (msg)
    {
    case SENSOR_INT_U18_A:
    case SENSOR_INT_U18_B:
      mcpU18.interruptedBy(portA, portB);
      if (portA > 0)
      {
        port = mcpU18.readPort(MCP23017Port::A);
        for (int i = 0; i < sizeof(uint8_t); i++)
        {
          if ((port & (1u << i)) > 0)
          {
          }
        }
      }
      if (portB > 0)
      {
        port = mcpU18.readPort(MCP23017Port::B);
      }
      break;

      break;
    // case SENSOR_INT_U35_A:
    //   port = mcpU35.readPort(MCP23017Port::A);
    //   break;
    // case SENSOR_INT_U35_B:
    //   port = mcpU35.readPort(MCP23017Port::B);
    //   break;
    default:
      break;
    }
    memset(&msg, 0, sizeof(msg));
  }
}

static void initSensorMcps(MCP23017 *mcp)
{
  mcp->init();
  mcp->portMode(MCP23017Port::A, 0b11111111); // Port A as input
  mcp->portMode(MCP23017Port::B, 0b11111111); // Port B as input

  mcp->interruptMode(MCP23017InterruptMode::Separated);
  mcp->interrupt(MCP23017Port::A, FALLING);
  mcp->interrupt(MCP23017Port::B, FALLING);

  mcp->writeRegister(MCP23017Register::IPOL_A, 0x00);
  mcp->writeRegister(MCP23017Register::IPOL_B, 0x00);

  mcp->writeRegister(MCP23017Register::GPIO_A, 0x00);
  mcp->writeRegister(MCP23017Register::GPIO_B, 0x00);

  mcp->clearInterrupts();
}

void setup()
{
  Wire.begin(I2C_SDA, I2C_SCL, I2C_FRQ);

  pinMode(SENSOR_U18_INTA, INPUT_PULLUP);
  pinMode(SENSOR_U18_INTB, INPUT_PULLUP);
  pinMode(SENSOR_U35_INTA, INPUT_PULLUP);
  pinMode(SENSOR_U35_INTB, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SENSOR_U18_INTA), sensorIntU18A, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_U18_INTB), sensorIntU18B, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_U35_INTA), sensorIntU35A, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_U35_INTB), sensorIntU35B, FALLING);

  initSensorMcps(&mcpU18);
  initSensorMcps(&mcpU36);

  for (Shutter s : shutters)
  {
    s.onStart();
  }
}

void loop()
{
  // do nothing here, as software runs in dedicated tasks
  uint8_t u18PortA;
  uint8_t u18PortB;
  uint8_t u36PortA;
  uint8_t u36PortB;

  if (interrupt)
  {
    noInterrupts();
    mcpU18.interruptedBy(u18PortA, u18PortB);
    mcpU36.interruptedBy(u36PortA, u36PortB);
    interrupt = false;
    interrupts();

    struct Msg msg;
    for (int i = 0; i < 8; i++)
    {
      if ((u18PortA & (1u << i)) > 0)
      {
        for (Shutter s : shutters)
        {
          if (s.sensor.getUp() == static_cast<MCP23017Pin::Names>(i))
          {
            msg.evt = SHUTTER_UP_EVT;
            s.onEvent(&msg);
          }

          else if (s.sensor.getDown() == static_cast<MCP23017Pin::Names>(i))
          {
            msg.evt = SHUTTER_DOWN_EVT;
            s.onEvent(&msg);
          }

          else
          {
          }
        }
      }
    }

    for (int i = 0; i < 8; i++)
    {
      if ((u18PortB & (1u << i)) > 0)
      {
        for (Shutter s : shutters)
        {
          if (s.sensor.getUp() == static_cast<MCP23017Pin::Names>(i) + MCP23017Pin::GPB0)
          {
            msg.evt = SHUTTER_UP_EVT;
            s.onEvent(&msg);
          }

          else if (s.sensor.getDown() == static_cast<MCP23017Pin::Names>(i) + MCP23017Pin::GPB0)
          {
            msg.evt = SHUTTER_DOWN_EVT;
            s.onEvent(&msg);
          }

          else
          {
          }
        }
      }
    }
  }
}