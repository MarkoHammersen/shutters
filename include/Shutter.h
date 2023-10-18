#ifndef __SHUTTER_H__
#define __SHUTTER_H__

#include <string>
using namespace std;

#include <arduino-timer.h>
#include <assert.h>
#include "hsm.hpp"
#include "Sensor.h"
#include "Actuator.h"
#include "esp_log.h"

class Shutter : private Hsm
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

    Timer<> _timer;
    bool _timeout;

    Msg const *_topHndlr(Msg const *msg);
    Msg const *_idleHndlr(Msg const *msg);
    Msg const *_stopHndlr(Msg const *msg);
    Msg const *_runningHndlr(Msg const *msg);
    Msg const *_upHndlr(Msg const *msg);
    Msg const *_downHndlr(Msg const *msg);


public:
    Shutter(string room, string dir, Sensor s, Actuator a);
    Sensor sensor;
    Actuator actuator;

    void startHsm();
    void processSensorEvents();
    void processSensorDebouncing();
};

#endif // __SHUTTER_H__