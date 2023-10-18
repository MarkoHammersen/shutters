#include "Shutter.h"

Shutter::Shutter(string room,
                 string dir,
                 Sensor sensor,
                 Actuator actuator)
    : Hsm("shutter", (EvtHndlr)&Shutter::topHndlr),
      _idle("idle", &top, (EvtHndlr)&Shutter::idleHndlr),
      _up("up", &top, (EvtHndlr)&Shutter::upHndlr),
      _down("down", &top, (EvtHndlr)&Shutter::downHndlr),
      _stop("stop", &top, (EvtHndlr)&Shutter::stopHndlr),
      _running("running", &top, (EvtHndlr)&Shutter::runningHndlr)
{
    _room = room;
    _dir = dir;
    sensor = sensor;
    actuator = actuator;
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
        _cmd = ShutterCmd::SWITCH_DOWN_ON;
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
        _cmd = ShutterCmd::STOP;
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
        _cmd = ShutterCmd::SWITCH_UP_ON;
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

ShutterCmd Shutter::getShutterCmd()
{
    return _cmd;
}
