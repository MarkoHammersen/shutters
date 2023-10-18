#include "Shutter.h"

typedef enum HsmEvents
{
    SENSOR_UP_EVT,
    SENSOR_DOWN_EVT,
    TIMEOUT_EVT
} hsmEvent_t;

using namespace std;
#define TAG "shutter"

extern void actuatorCmd(uint32_t i2cAddr, MCP23017Pin::Names pin, uint8_t state);
extern uint8_t sensorGetSinglePinEvent(uint32_t i2cAddr, MCP23017Pin::Names pin);

static bool _timeoutDebounce(void *arg)
{
    *(bool *)arg = true;
    return false; // do not repeat timer
}

Shutter::Shutter(string room,
                 string dir,
                 Sensor sensor,
                 Actuator actuator)
    : Hsm("shutter", (EvtHndlr)&Shutter::_topHndlr),
      _idle("idle", &top, (EvtHndlr)&Shutter::_idleHndlr),
      _up("up", &top, (EvtHndlr)&Shutter::_upHndlr),
      _down("down", &top, (EvtHndlr)&Shutter::_downHndlr),
      _stop("stop", &top, (EvtHndlr)&Shutter::_stopHndlr),
      _running("running", &top, (EvtHndlr)&Shutter::_runningHndlr)
{
    _room = room;
    _dir = dir;
    sensor = sensor;
    actuator = actuator;

    // call the toggle_led function every 1000 millis (1 second)
    auto _timer = timer_create_default();
    _timeout = false;
}

Msg const *Shutter::_downHndlr(Msg const *msg)
{
    switch (msg->evt)
    {
    case START_EVT:
        ESP_LOGI(TAG, "down-INIT");
        return 0;

    case ENTRY_EVT:
        ESP_LOGI(TAG, "down-ENTRY");
        actuatorCmd(actuator.getI2cAddr(), actuator.getUp(), LOW);
        actuatorCmd(actuator.getI2cAddr(), actuator.getDown(), HIGH);
        _timer.in(HSM_DEBOUNCE_TIME, _timeoutDebounce, &_timeout);
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "down-EXIT");
        return 0;

    case TIMEOUT_EVT:
        ESP_LOGI(TAG, "down-TIMEOUT");
        STATE_TRAN(&_running);
        break;

    default:
        return 0;
    }
    return msg;
}

Msg const *Shutter::_stopHndlr(Msg const *msg)
{
    switch (msg->evt)
    {
    case START_EVT:
        ESP_LOGI(TAG, "stop-INIT");
        return 0;

    case ENTRY_EVT:
        ESP_LOGI(TAG, "stop-ENTRY");
        actuatorCmd(actuator.getI2cAddr(), actuator.getUp(), LOW);
        actuatorCmd(actuator.getI2cAddr(), actuator.getDown(), LOW);
        _timer.in(HSM_DEBOUNCE_TIME, _timeoutDebounce);
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "stop-EXIT");
        return 0;

    case TIMEOUT_EVT:
        ESP_LOGI(TAG, "stop-TIMEOUT");
        STATE_TRAN(&_idle);
        break;        
    }
    return msg;
}

Msg const *Shutter::_idleHndlr(Msg const *msg)
{
    switch (msg->evt)
    {
    case START_EVT:
        ESP_LOGI(TAG, "idle-INIT");
        return 0;

    case ENTRY_EVT:
        ESP_LOGI(TAG, "idle-ENTRY");
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "idle-EXIT");
        return 0;

    case SENSOR_UP_EVT:
        ESP_LOGI(TAG, "idle-SENSOR_UP_EVT");
        STATE_TRAN(&_up);
        return 0;

    case SENSOR_DOWN_EVT:
        ESP_LOGI(TAG, "idle-SENSOR_DOWN_EVT");
        STATE_TRAN(&_down);
        return 0;        
    }
    return msg;
}

Msg const *Shutter::_upHndlr(Msg const *msg)
{
    switch (msg->evt)
    {
    case START_EVT:
        ESP_LOGI(TAG, "up-INIT");
        return 0;

    case ENTRY_EVT:
        ESP_LOGI(TAG, "up-ENTRY");
        actuatorCmd(actuator.getI2cAddr(), actuator.getDown(), LOW);
        actuatorCmd(actuator.getI2cAddr(), actuator.getUp(), HIGH);
        _timer.in(HSM_DEBOUNCE_TIME, _timeoutDebounce);
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "up-EXIT");
        return 0;

    case TIMEOUT_EVT:
        ESP_LOGI(TAG, "up-TIMEOUT");
        STATE_TRAN(&_running);
        break;

    default:
        return 0;
    }
    return msg;
}

Msg const *Shutter::_topHndlr(Msg const *msg)
{
    switch (msg->evt)
    {
    case START_EVT:
        ESP_LOGI(TAG, "top-INIT");
        return 0;
    case ENTRY_EVT:
        ESP_LOGI(TAG, "top-ENTRY");
        return 0;
    case EXIT_EVT:
        ESP_LOGI(TAG, "top-EXIT");
        return 0;
    }
    return msg;
}

Msg const *Shutter::_runningHndlr(Msg const *msg)
{
    switch (msg->evt)
    {
    case START_EVT:
        ESP_LOGI(TAG, "running-INIT");
        return 0;
    case ENTRY_EVT:
        ESP_LOGI(TAG, "running-ENTRY");
        return 0;
    case EXIT_EVT:
        ESP_LOGI(TAG, "running-EXIT");
        return 0;
    case SENSOR_DOWN_EVT:
        ESP_LOGI(TAG, "running-SENSOR_DOWN_EVT");
        STATE_TRAN(&_stop);
        break;
    case SENSOR_UP_EVT:
        ESP_LOGI(TAG, "running-SENSOR_UP_EVT");
        STATE_TRAN(&_stop);
        break;
    default:
        return 0;
    }
    return msg;
}

void Shutter::processSensorEvents()
{
    if(1 == sensorGetSinglePinEvent(sensor.getI2cAddr(), sensor.getUp()))
    {
        Msg const msg = {SENSOR_UP_EVT};
        onEvent(&msg);
    }

    if(1 == sensorGetSinglePinEvent(sensor.getI2cAddr(), sensor.getDown()))
    {
        Msg const  msg = {SENSOR_DOWN_EVT};
        onEvent(&msg);
    }
}

void Shutter::processSensorDebouncing()
{
    if(_timeout)
    {
        _timeout = false;
        Msg const msg = {TIMEOUT_EVT};
        onEvent(&msg);
    }
}

void Shutter::startHsm()
{
    onStart();
}
