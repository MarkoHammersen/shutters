#include <Arduino.h>
#include <arduino-timer.h>
#include "esp_log.h"
#include "Message.h"
#include "PinSetup.h"
#include "Window.h"
#include "hsm.hpp"
#include "Shutter.h"

enum ShutterEvents {
  SENSOR_UP_EVT,
  SENSOR_DOWN_EVT,
  TIMEOUT_EVT
};

const Msg hsmMsgs[] = {
  SENSOR_UP_EVT,
  SENSOR_DOWN_EVT,
  TIMEOUT_EVT
};

typedef enum HsmEvents
{

} hsmEvent_t;

#define TAG "shutter"

QueueHandle_t qHandleShutters = NULL;
static Shutter shutters[] =
{
//        /*           Name	            Input				                                                      Output
//                  Name              (I2C MSP23017	        Up	                Down)	                        (I2C MSP23017	          Up	                Down) */
  Shutter(Window::LI_EAST, PinSetup(I2C_ADDR_SENSOR_U18, MCP23017Pin::GPB0, MCP23017Pin::GPB1), PinSetup(I2C_ADDR_ACTUATOR_U36, MCP23017Pin::GPB0, MCP23017Pin::GPB0))
//        // Shutter("Li", "South",     Sensor(Port("U36",  "B2"), Port("U36",  "B3")), Actuator(Port("U4",  "A4"), Port("A4", "A3"))),
//        // Shutter("Hall", "South",     Sensor(Port("U36",  "B4"), Port("U36",  "B5")), Actuator(Port("U4",  "A2"), Port("A2", "A1"))),
//        // Shutter("Laundry", "South",  Sensor(Port("U36",  "B6"), Port("U36",  "B7")), Actuator(Port("U4",  "A0"), Port("A0", "B6"))),
//        // Shutter("Bath", "South",      Sensor(Port("U36",  "A3"), Port("U36",  "A2")), Actuator(Port("U4",  "A0"), Port("B5", "B4"))),
//        // Shutter("Bath", "North",      Sensor(Port("U36",  "A1"), Port("U36",  "A0")), Actuator(Port("U4",  "B5"), Port("B3", "B2"))),
//        // Shutter("Ju", "West",    Sensor(Port("U1",   "B0"), Port("U1",   "B1")), Actuator(Port("U4",  "B3"), Port("B1", "B0"))),
//        // Shutter("Ju", "North",    Sensor(Port("U1",   "B2"), Port("U1",   "B3")), Actuator(Port("U37", "B1"), Port("A6", "A5"))),
//        // Shutter("Pa", "North",     Sensor(Port("U1",   "B4"), Port("U1",   "B5")), Actuator(Port("U37", "B1"), Port("A4", "A3"))),
//        // Shutter("Pa", "East",      Sensor(Port("U1",   "B6"), Port("U1",   "B7")), Actuator(Port("U37", "B1"), Port("A2", "A1"))),
//        // Shutter("Office", "East",     Sensor(Port("U1",   "A7"), Port("U1",   "A6")), Actuator(Port("U37", "B1"), Port("B6", "B5")))
};

static bool _onTimeout(void *arg)
{
    /* (uint8_t)arg is the shutter window */
    appMessage_t msg;
    msg.evt = TIMEOUT;
    msg.data = *(uint8_t *)arg;
    xQueueSend(qHandleShutters, &msg, portMAX_DELAY);
    return false; // do not repeat timer
}

Shutter::Shutter(Window window, PinSetup sensor, PinSetup actuator)
    : Hsm("shutter", (EvtHndlr)&Shutter::topHndlr),
      _idle("idle", &top, (EvtHndlr)&Shutter::idleHndlr),
      _stop("stop", &top, (EvtHndlr)&Shutter::stopHndlr),
      _running("running", &top, (EvtHndlr)&Shutter::runningHndlr),
      _up("up", &top, (EvtHndlr)&Shutter::upHndlr),
      _down("down", &top, (EvtHndlr)&Shutter::downHndlr)      
{
    _window = window;
    _sensor = sensor;
    _actuator = actuator;

    _timer = timer_create_default();
}

Msg const *Shutter::downHndlr(Msg const *msg)
{
    switch (msg->evt)
    {
    case START_EVT:
        ESP_LOGI(TAG, "%d: down-INIT", (int)_window);
        return 0;

    case ENTRY_EVT:
    {
        ESP_LOGI(TAG, "%d: down-ENTRY", (int)_window);

        appMessage_t msg;
        msg.i2cAddr = _actuator.getI2cAddr();
        msg.evt = STOP;
        msg.data = _actuator.getUp();
        xQueueSend(qHandleActuators, &msg, portMAX_DELAY);
        msg.evt = RUN;
        msg.data = _actuator.getDown();
        xQueueSend(qHandleActuators, &msg, portMAX_DELAY);
        uint8_t num = static_cast<uint8_t>(_window);
        _timer.in(HSM_DEBOUNCE_TIME, _onTimeout, &num);
    }
        return 0;

    case TIMEOUT_EVT:
        ESP_LOGI(TAG, "%d: down-TIMEOUT", (int)_window);
        STATE_TRAN(&_running);
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "%d: down-EXIT", (int)_window);
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
        ESP_LOGI(TAG, "%d: stop-INIT", (int)_window);
        return 0;

    case ENTRY_EVT:
        ESP_LOGI(TAG, "%d: stop-ENTRY", (int)_window);

        appMessage_t msg;
        msg.i2cAddr = _actuator.getI2cAddr();
        msg.evt = STOP;
        msg.data = _actuator.getUp();
        xQueueSend(qHandleActuators, &msg, portMAX_DELAY);
        msg.data = _actuator.getDown();        
        xQueueSend(qHandleActuators, &msg, portMAX_DELAY);

        num = static_cast<uint8_t>(_window);
        _timer.in(HSM_DEBOUNCE_TIME, _onTimeout, &num);
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "%d: stop-EXIT", (int)_window);
        return 0;

    case TIMEOUT_EVT:
        ESP_LOGI(TAG, "%d: stop-TIMEOUT", (int)_window);
        STATE_TRAN(&_idle);
        return 0;
    }
    return msg;
}

Msg const *Shutter::idleHndlr(Msg const *msg)
{
    switch (msg->evt)
    {
    case START_EVT:
        ESP_LOGI(TAG, "%d: idle-INIT", (int)_window);
        return 0;

    case ENTRY_EVT:
        ESP_LOGI(TAG, "%d: idle-ENTRY", (int)_window);
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "%d: idle-EXIT", (int)_window);
        return 0;

    case SENSOR_UP_EVT:
        ESP_LOGI(TAG, "%d: idle-SENSOR_UP_EVT", (int)_window);
        STATE_TRAN(&_up);
        return 0;

    case SENSOR_DOWN_EVT:
        ESP_LOGI(TAG, "%d: idle-SENSOR_DOWN_EVT", (int)_window);
        STATE_TRAN(&_down);
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
        ESP_LOGI(TAG, "%d: up-INIT", (int)_window);
        return 0;

    case ENTRY_EVT:
        ESP_LOGI(TAG, "%d: up-ENTRY", (int)_window);

        appMessage_t msg;
        msg.i2cAddr = _actuator.getI2cAddr();
        msg.evt = STOP;
        msg.data = _actuator.getDown();
        xQueueSend(qHandleActuators, &msg, portMAX_DELAY);
        msg.evt = RUN;
        msg.data = _actuator.getUp();
        xQueueSend(qHandleActuators, &msg, portMAX_DELAY);
        num = static_cast<uint8_t>(_window);
        _timer.in(HSM_DEBOUNCE_TIME, _onTimeout, &num);
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "%d: up-EXIT", (int)_window);
        return 0;

    case TIMEOUT_EVT:
        ESP_LOGI(TAG, "%d: up-TIMEOUT", (int)_window);
        STATE_TRAN(&_running);
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
        ESP_LOGI(TAG, "%d: top-INIT", (int)_window);
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
        ESP_LOGI(TAG, "%d: running-INIT", (int)_window);
        return 0;
    case ENTRY_EVT:
    {
        ESP_LOGI(TAG, "%d: running-ENTRY", (int)_window);
        uint8_t num = static_cast<uint8_t>(_window);
        _timer.in(HSM_RUN_TIME, _onTimeout, &num);
    }
        return 0;
    case EXIT_EVT:
        ESP_LOGI(TAG, "%d: running-EXIT", (int)_window);
        return 0;
    case SENSOR_DOWN_EVT:
        ESP_LOGI(TAG, "%d: running-SENSOR_DOWN_EVT", (int)_window);
        STATE_TRAN(&_stop);
        return 0;

    case SENSOR_UP_EVT:
        ESP_LOGI(TAG, "%d: running-SENSOR_UP_EVT", (int)_window);
        STATE_TRAN(&_stop);
        return 0;

    case TIMEOUT_EVT:
        ESP_LOGI(TAG, "%d: running-TIMEOUT_EVT", (int)_window);
        STATE_TRAN(&_stop);
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
    case RUN:
        if (_sensor.getI2cAddr() == msg->i2cAddr)
        {
            if (_sensor.getUp() == msg->data)
            {
                onEvent(&hsmMsgs[SENSOR_UP_EVT]);
            }
            else if (_sensor.getDown() == msg->data)
            {
                onEvent(&hsmMsgs[SENSOR_DOWN_EVT]);
            }
            else
            {
            }
        }
        break;

    case TIMEOUT:
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

    for (size_t i = 0; i < sizeof(shutters)/sizeof(*shutters); i++)
    {
        shutters[i].startHsm();
    }

    while (1)
    {
        if (pdTRUE == xQueueReceive(qHandleShutters, &msg, portMAX_DELAY))
        {
            for (size_t i = 0; i < sizeof(shutters) / sizeof(*shutters); i++)
            {
              shutters[i].processMsg(&msg);
            }
        }
    }
}

void initShutters()
{
    TaskHandle_t xHandleShutters = NULL;
    xTaskCreate(vTaskShutter, "shutter", 4096, NULL, 10, &xHandleShutters);
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
