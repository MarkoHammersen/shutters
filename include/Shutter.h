#ifndef __SHUTTER_H__
#define __SHUTTER_H__

#include <string>
using namespace std;

#include "DeviceNames.h"
#include "hsm.hpp"
#include "Sensor.h"
#include "Actuator.h"

enum class ShutterCmd : uint8_t
{
    IDLE,
	SWITCH_UP_ON,
	SWITCH_DOWN_ON,
    STOP,
};

typedef enum HsmEvents
{
    SENSOR_UP_EVT,
    SENSOR_DOWN_EVT,
    TIMEOUT_EVT
} hsmEvent_t;

class Shutter : public Hsm
{
protected:
    string _room;
    string _dir;

    State _top;
    State _idle;
    State _stop;
    State _running;
    State _up;
    State _down;

    ShutterCmd _cmd;

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

    Sensor sensor;
    Actuator actuator;

    ShutterCmd getShutterCmd();
};

#endif // __SHUTTER_H__