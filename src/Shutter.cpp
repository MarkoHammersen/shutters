#include <Arduino.h>
#include <arduino-timer.h>
#include "esp_log.h"
#include "Message.h"
#include "PinSetup.h"
#include "Window.h"
#include "hsm.hpp"
#include "Shutter.h"

#define SHTTR_LOG log_i

enum ShutterEvents
{
  SENSOR_UP_EVT,
  SENSOR_DOWN_EVT,
  TIMEOUT_EVT
};

const Msg hsmMsgs[] = {
    SENSOR_UP_EVT,
    SENSOR_DOWN_EVT,
    TIMEOUT_EVT};

typedef enum HsmEvents
{

} hsmEvent_t;

#define TAG "shutter"

QueueHandle_t qHandleShutters = NULL;

static Shutter shutters[static_cast<uint8_t>(Window::Count)] = {
    Shutter(Window::HALL, 120000,
            PinSetup(I2C_ADDR_SENSOR_U18, MCP23017Pin::GPB0, MCP23017Pin::GPB1),    // sensor
            PinSetup(I2C_ADDR_ACTUATOR_U36, MCP23017Pin::GPB0, MCP23017Pin::GPB1)), // actuator

    Shutter(Window::LAUNDRY, 120000,
            PinSetup(I2C_ADDR_SENSOR_U18, MCP23017Pin::GPB2, MCP23017Pin::GPB3),    // sensor
            PinSetup(I2C_ADDR_ACTUATOR_U36, MCP23017Pin::GPB2, MCP23017Pin::GPB3)), // actuator

    Shutter(Window::BATH_SOUTHWEST, 20000,
            PinSetup(I2C_ADDR_SENSOR_U18, MCP23017Pin::GPB4, MCP23017Pin::GPB5),    // sensor
            PinSetup(I2C_ADDR_ACTUATOR_U36, MCP23017Pin::GPB4, MCP23017Pin::GPB5)), // actuator

    Shutter(Window::BATH_NORTHWEST, 12000,
            PinSetup(I2C_ADDR_SENSOR_U18, MCP23017Pin::GPA0, MCP23017Pin::GPA1),    // sensor
            PinSetup(I2C_ADDR_ACTUATOR_U36, MCP23017Pin::GPA0, MCP23017Pin::GPA1)), // actuator

    Shutter(Window::JU_WEST, 12000,
            PinSetup(I2C_ADDR_SENSOR_U18, MCP23017Pin::GPA2, MCP23017Pin::GPA3),    // sensor
            PinSetup(I2C_ADDR_ACTUATOR_U36, MCP23017Pin::GPA2, MCP23017Pin::GPA3)), // actuator

    Shutter(Window::JU_NORTH, 12000,
            PinSetup(I2C_ADDR_SENSOR_U18, MCP23017Pin::GPA4, MCP23017Pin::GPA5),    // sensor
            PinSetup(I2C_ADDR_ACTUATOR_U37, MCP23017Pin::GPB0, MCP23017Pin::GPB1)), // actuator

    Shutter(Window::PA_NORTH, 12000,
            PinSetup(I2C_ADDR_SENSOR_U2, MCP23017Pin::GPB0, MCP23017Pin::GPB1),     // sensor
            PinSetup(I2C_ADDR_ACTUATOR_U37, MCP23017Pin::GPB2, MCP23017Pin::GPB3)), // actuator

    Shutter(Window::PA_EAST, 12000,
            PinSetup(I2C_ADDR_SENSOR_U2, MCP23017Pin::GPB2, MCP23017Pin::GPB3),     // sensor
            PinSetup(I2C_ADDR_ACTUATOR_U37, MCP23017Pin::GPA0, MCP23017Pin::GPA1)), // actuator

    Shutter(Window::OFFICE, 12000,
            PinSetup(I2C_ADDR_SENSOR_U2, MCP23017Pin::GPB4, MCP23017Pin::GPB5),     // sensor
            PinSetup(I2C_ADDR_ACTUATOR_U37, MCP23017Pin::GPA2, MCP23017Pin::GPA3)), // actuator

    Shutter(Window::LI_EAST, 12000,
            PinSetup(I2C_ADDR_SENSOR_U2, MCP23017Pin::GPA0, MCP23017Pin::GPA1),     // sensor
            PinSetup(I2C_ADDR_ACTUATOR_U37, MCP23017Pin::GPA7, MCP23017Pin::GPA6)), // actuator

    Shutter(Window::LI_SOUTH, 12000,
            PinSetup(I2C_ADDR_SENSOR_U2, MCP23017Pin::GPA2, MCP23017Pin::GPA3),     // sensor
            PinSetup(I2C_ADDR_ACTUATOR_U37, MCP23017Pin::GPA5, MCP23017Pin::GPA4)), // actuator
};

static void sendToActuator(appMessage_t *msg)
{
  xQueueSend(qHandleActuators, msg, portMAX_DELAY);
}

static void startTimer(TimerHandle_t h, uint32_t timeout_ms)
{
  if (xTimerIsTimerActive(h) != pdFALSE)
  {
    // xTimer is already active - stop it.
    xTimerStop(h, portMAX_DELAY);
  }
  assert(pdPASS == xTimerChangePeriod(h, timeout_ms / portTICK_PERIOD_MS, portMAX_DELAY) == pdPASS);
  assert(pdPASS == xTimerStart(h, portMAX_DELAY) == pdPASS);
}

static void onTimeout(TimerHandle_t xTimer)
{
  /* (uint8_t)arg is the shutter window */
  appMessage_t msg;
  msg.evt = AppEvents::TIMEOUT;

  uint32_t i = 0;
  for (i = 0; i < static_cast<uint8_t>(Window::Count); i++)
  {
    if (shutters[i].getTimerHandle() == xTimer)
    {
      msg.data = static_cast<uint16_t>(shutters[i].getWindow());
      break;
    }
  }
  if (i >= static_cast<uint8_t>(Window::Count))
  {
    log_e("xTimer invalid");
    return;
  }

  xQueueSend(qHandleShutters, &msg, portMAX_DELAY);
}

Shutter::Shutter(Window window, uint32_t runTime, PinSetup sensor, PinSetup actuator)
    : Hsm("shutter", static_cast<EvtHndlr>(&Shutter::topHndlr)),
      _idle("idle", &top, static_cast<EvtHndlr>(&Shutter::idleHndlr)),
      _stop("stop", &top, static_cast<EvtHndlr>(&Shutter::stopHndlr)),
      _running("running", &top, static_cast<EvtHndlr>(&Shutter::runningHndlr)),
      _up("up", &top, static_cast<EvtHndlr>(&Shutter::upHndlr)),
      _down("down", &top, static_cast<EvtHndlr>(&Shutter::downHndlr))
{
  _window = window;
  _sensor = sensor;
  _actuator = actuator;
  _runTime = runTime;

  _timer = xTimerCreate(NULL,
                        HSM_DEBOUNCE_TIME / portTICK_PERIOD_MS, // will be changed before every start
                        // The timers will not auto-reload themselves when they expire.
                        pdFALSE,
                        (void *)_window,
                        onTimeout);
  assert(_timer != NULL);
}

Msg const *Shutter::downHndlr(Msg const *msg)
{
  switch (msg->evt)
  {
  case START_EVT:
    SHTTR_LOG("%s: down-INIT", windowName[(uint8_t)_window]);
    return 0;

  case ENTRY_EVT:
  {
    SHTTR_LOG("%s: down-ENTRY", windowName[(uint8_t)_window]);

    appMessage_t msg;
    msg.i2cAddr = _actuator.getI2cAddr();
    msg.evt = AppEvents::STOP;
    msg.data = _actuator.getUp();
    sendToActuator(&msg);
    msg.evt = AppEvents::RUN;
    msg.data = _actuator.getDown();
    sendToActuator(&msg);
    startTimer(_timer, HSM_DEBOUNCE_TIME);
  }
    return 0;

  case TIMEOUT_EVT:
    STATE_TRAN(&_running);
    SHTTR_LOG("%s: down-TIMEOUT", windowName[(uint8_t)_window]);

    return 0;

  case EXIT_EVT:
    SHTTR_LOG("%s: down-EXIT", windowName[(uint8_t)_window]);
    return 0;

  default:
    return 0;
  }
  return msg;
}

Msg const *Shutter::stopHndlr(Msg const *msg)
{
  uint8_t num;
  switch (msg->evt)
  {
  case START_EVT:
    SHTTR_LOG("%s: stop-INIT", windowName[(uint8_t)_window]);
    return 0;

  case ENTRY_EVT:
    SHTTR_LOG("%s: stop-ENTRY", windowName[(uint8_t)_window]);

    appMessage_t msg;
    msg.i2cAddr = _actuator.getI2cAddr();
    msg.evt = AppEvents::STOP;
    msg.data = _actuator.getUp();
    sendToActuator(&msg);
    msg.data = _actuator.getDown();
    sendToActuator(&msg);
    startTimer(_timer, HSM_DEBOUNCE_TIME);

    return 0;

  case EXIT_EVT:
    SHTTR_LOG("%s: stop-EXIT", windowName[(uint8_t)_window]);
    return 0;

  case TIMEOUT_EVT:
    STATE_TRAN(&_idle);
    SHTTR_LOG("%s: stop-TIMEOUT", windowName[(uint8_t)_window]);

    return 0;
  }
  return msg;
}

Msg const *Shutter::idleHndlr(Msg const *msg)
{
  switch (msg->evt)
  {
  case START_EVT:
    SHTTR_LOG("%s: idle-INIT", windowName[(uint8_t)_window]);
    return 0;

  case ENTRY_EVT:
    SHTTR_LOG("%s: idle-ENTRY", windowName[(uint8_t)_window]);
    return 0;

  case EXIT_EVT:
    SHTTR_LOG("%s: idle-EXIT", windowName[(uint8_t)_window]);
    return 0;

  case SENSOR_UP_EVT:
    STATE_TRAN(&_up);
    SHTTR_LOG("%s: idle-SENSOR_UP_EVT", windowName[(uint8_t)_window]);

    return 0;

  case SENSOR_DOWN_EVT:
    STATE_TRAN(&_down);
    SHTTR_LOG("%s: idle-SENSOR_DOWN_EVT", windowName[(uint8_t)_window]);

    return 0;
  }
  return msg;
}

Msg const *Shutter::upHndlr(Msg const *msg)
{
  uint8_t num;
  switch (msg->evt)
  {
  case START_EVT:
    SHTTR_LOG("%s: up-INIT", windowName[(uint8_t)_window]);
    return 0;

  case ENTRY_EVT:
    SHTTR_LOG("%s: up-ENTRY", windowName[(uint8_t)_window]);

    appMessage_t msg;
    msg.i2cAddr = _actuator.getI2cAddr();
    msg.evt = AppEvents::STOP;
    msg.data = _actuator.getDown();
    sendToActuator(&msg);
    msg.evt = AppEvents::RUN;
    msg.data = _actuator.getUp();
    sendToActuator(&msg);
    startTimer(_timer, HSM_DEBOUNCE_TIME);
    return 0;

  case EXIT_EVT:
    SHTTR_LOG("%s: up-EXIT", windowName[(uint8_t)_window]);
    return 0;

  case TIMEOUT_EVT:
    STATE_TRAN(&_running);
    SHTTR_LOG("%s: up-TIMEOUT", windowName[(uint8_t)_window]);
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
    SHTTR_LOG("%s: top-INIT", windowName[(uint8_t)_window]);
    STATE_START(&_idle);
    return 0;
  }
  return msg;
}

Msg const *Shutter::runningHndlr(Msg const *msg)
{
  switch (msg->evt)
  {
  case START_EVT:
    SHTTR_LOG("%s: running-INIT", windowName[(uint8_t)_window]);
    return 0;
  case ENTRY_EVT:
  {
    SHTTR_LOG("%s: running-ENTRY", windowName[(uint8_t)_window]);
    startTimer(_timer,getRunTime());
  }
    return 0;

  case EXIT_EVT:
    SHTTR_LOG("%s: running-EXIT", windowName[(uint8_t)_window]);
    return 0;

  case SENSOR_DOWN_EVT:
    STATE_TRAN(&_stop);
    SHTTR_LOG("%s: running-SENSOR_DOWN_EVT", windowName[(uint8_t)_window]);
    return 0;

  case SENSOR_UP_EVT:
    STATE_TRAN(&_stop);
    SHTTR_LOG("%s: running-SENSOR_UP_EVT", windowName[(uint8_t)_window]);

    return 0;

  case TIMEOUT_EVT:
    STATE_TRAN(&_stop);
    SHTTR_LOG("%s: running-TIMEOUT_EVT", windowName[(uint8_t)_window]);

    return 0;

  default:
    return 0;
  }
  return msg;
}

void Shutter::processMsg(const appMessage_t *msg)
{
  switch (msg->evt)
  {
  case AppEvents::TOUCH:
    if (_sensor.getI2cAddr() == msg->i2cAddr)
    {
      if ((msg->data & ((uint16_t)1 << (_sensor.getUp()))) > 0)
      {
        onEvent(&hsmMsgs[SENSOR_UP_EVT]);
      }
      else if ((msg->data & ((uint16_t)1 << (_sensor.getDown()))) > 0)
      {
        onEvent(&hsmMsgs[SENSOR_DOWN_EVT]);
      }
      else
      {
      }
    }
    break;

  case AppEvents::TIMEOUT: // which is the same as event SENSOR_TOUCH_EVT
  {
    if (msg->data == static_cast<uint8_t>(_window))
    {
      onEvent(&hsmMsgs[TIMEOUT_EVT]);
    }
  }
  break;

  default:
    break;
  }
}

static void vTaskShutter(void *arg)
{
  appMessage_t msg;
  qHandleShutters = xQueueCreate(32, sizeof(msg));
  ESP_LOGI("Shutter", "task ENTRY");
  for (size_t i = 0; i < sizeof(shutters) / sizeof(*shutters); i++)
  {
    shutters[i].startHsm();
  }
  while (1)
  {
    if (pdTRUE == xQueueReceive(qHandleShutters, &msg, portMAX_DELAY))
    {
      for (size_t i = 0; i < sizeof(shutters) / sizeof(*shutters); i++)
      {
        delay(1); // debounce a little
        shutters[i].processMsg(&msg);
      }
    }
  }
}

void initShutterTask()
{
  TaskHandle_t xHandleShutters = NULL;
  assert(pdPASS == xTaskCreate(vTaskShutter, "shutter", 2 * 4096, NULL, 10, &xHandleShutters));
}

#ifdef UNIT_TEST
Shutter *getShutter(int i)
{
  if (i < getSizeOfShutters())
  {
    return &shutters[i];
  }
  return NULL;
}

int getSizeOfShutters()
{
  return ((int)(sizeof(shutters) / sizeof(*shutters)));
}
#endif // UNIT_TEST
