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

static bool _onTimeout(void *arg)
{
    *(bool *)arg = true;
    return false; // do not repeat timer
}

Shutter::Shutter(const char* room,
                 const char* dir,
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
        ESP_LOGI(TAG, "%s: %s: down-INIT", getRoom(), getDir());
        return 0;

    case ENTRY_EVT:
        ESP_LOGI(TAG, "%s: %s: down-ENTRY", getRoom(), getDir());
        actuatorCmd(actuator.getI2cAddr(), actuator.getUp(), LOW);
        actuatorCmd(actuator.getI2cAddr(), actuator.getDown(), HIGH);
        _timer.in(HSM_DEBOUNCE_TIME, _onTimeout, &_timeout);
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "%s: %s: down-EXIT", getRoom(), getDir());
        return 0;

    case TIMEOUT_EVT:
        ESP_LOGI(TAG, "%s: %s: down-TIMEOUT", getRoom(), getDir());
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
        ESP_LOGI(TAG, "%s: %s: stop-INIT", getRoom(), getDir());
        return 0;

    case ENTRY_EVT:
        ESP_LOGI(TAG, "%s: %s: stop-ENTRY", getRoom(), getDir());
        actuatorCmd(actuator.getI2cAddr(), actuator.getUp(), LOW);
        actuatorCmd(actuator.getI2cAddr(), actuator.getDown(), LOW);
        _timer.in(HSM_DEBOUNCE_TIME, _onTimeout);
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "%s: %s: stop-EXIT", getRoom(), getDir());
        return 0;

    case TIMEOUT_EVT:
        ESP_LOGI(TAG, "%s: %s: stop-TIMEOUT", getRoom(), getDir());
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
        ESP_LOGI(TAG, "%s: %s: idle-INIT", getRoom(), getDir());
        return 0;

    case ENTRY_EVT:
        ESP_LOGI(TAG, "%s: %s: idle-ENTRY", getRoom(), getDir());
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "%s: %s: idle-EXIT", getRoom(), getDir());
        return 0;

    case SENSOR_UP_EVT:
        ESP_LOGI(TAG, "%s: %s: idle-SENSOR_UP_EVT", getRoom(), getDir());
        STATE_TRAN(&_up);
        return 0;

    case SENSOR_DOWN_EVT:
        ESP_LOGI(TAG, "%s: %s: idle-SENSOR_DOWN_EVT", getRoom(), getDir());
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
        ESP_LOGI(TAG, "%s: %s: up-INIT", getRoom(), getDir());
        return 0;

    case ENTRY_EVT:
        ESP_LOGI(TAG, "%s: %s: up-ENTRY", getRoom(), getDir());
        actuatorCmd(actuator.getI2cAddr(), actuator.getDown(), LOW);
        actuatorCmd(actuator.getI2cAddr(), actuator.getUp(), HIGH);
        _timer.in(HSM_DEBOUNCE_TIME, _onTimeout);
        return 0;

    case EXIT_EVT:
        ESP_LOGI(TAG, "%s: %s: up-EXIT", getRoom(), getDir());
        return 0;

    case TIMEOUT_EVT:
        ESP_LOGI(TAG, "%s: %s: up-TIMEOUT", getRoom(), getDir());
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
        ESP_LOGI(TAG, "%s: %s: top-INIT", getRoom(), getDir());
        return 0;
    case ENTRY_EVT:
        ESP_LOGI(TAG, "%s: %s: top-ENTRY", getRoom(), getDir());
        return 0;
    case EXIT_EVT:
        ESP_LOGI(TAG, "%s: %s: top-EXIT", getRoom(), getDir());
        return 0;
    }
    return msg;
}

Msg const *Shutter::_runningHndlr(Msg const *msg)
{
    switch (msg->evt)
    {
    case START_EVT:
        ESP_LOGI(TAG, "%s: %s: running-INIT", getRoom(), getDir());
        return 0;
    case ENTRY_EVT:
        ESP_LOGI(TAG, "%s: %s: running-ENTRY", getRoom(), getDir());
        _timer.in(HSM_RUN_TIME, _onTimeout);
        return 0;
    case EXIT_EVT:
        ESP_LOGI(TAG, "%s: %s: running-EXIT", getRoom(), getDir());
        return 0;
    case SENSOR_DOWN_EVT:
        ESP_LOGI(TAG, "%s: %s: running-SENSOR_DOWN_EVT", getRoom(), getDir());
        STATE_TRAN(&_stop);
        break;
    case SENSOR_UP_EVT:
        ESP_LOGI(TAG, "%s: %s: running-SENSOR_UP_EVT", getRoom(), getDir());
        STATE_TRAN(&_stop);
    case TIMEOUT_EVT:
        ESP_LOGI(TAG, "%s: %s: running-TIMEOUT_EVT", getRoom(), getDir());
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
