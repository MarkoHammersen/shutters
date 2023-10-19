#ifndef __SHUTTER_H__
#define __SHUTTER_H__

#include <arduino-timer.h>
#include "hsm.hpp"
#include "Message.h"
#include "PinSetup.h"
#include "esp_log.h"
#include "Window.h"


class Shutter : private Hsm
{
protected:
    Window _window;

    State _top;
    State _idle;
    State _stop;
    State _running;
    State _up;
    State _down;

    PinSetup _sensor;
    PinSetup _actuator;

    Timer<> _timer;

    Msg const *_topHndlr(Msg const *msg);
    Msg const *_idleHndlr(Msg const *msg);
    Msg const *_stopHndlr(Msg const *msg);
    Msg const *_runningHndlr(Msg const *msg);
    Msg const *_upHndlr(Msg const *msg);
    Msg const *_downHndlr(Msg const *msg);

public:
    Shutter(Window w, PinSetup sensor, PinSetup actuator);
    void startHsm(){ onStart(); };
    void processMsg(const appMessage_t *msg);
};

void initShutters();

#endif // __SHUTTER_H__