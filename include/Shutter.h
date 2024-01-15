#ifndef __SHUTTER_H__
#define __SHUTTER_H__

class Shutter : public Hsm
{
protected:
    Window _window;
    PinSetup _sensor;
    PinSetup _actuator;

    State _top;
    State _idle;
    State _stop;
    State _running;
    State _up;
    State _down;

    Timer<> _timer;

    Msg const *topHndlr(Msg const *msg);
    Msg const *idleHndlr(Msg const *msg);
    Msg const *stopHndlr(Msg const *msg);
    Msg const *runningHndlr(Msg const *msg);
    Msg const *upHndlr(Msg const *msg);
    Msg const *downHndlr(Msg const *msg);

public:
    Shutter(Window w, PinSetup sensor, PinSetup actuator);
    void startHsm(){ onStart(); };
    char const* getCurrName() { return getName(); };
    void processMsg(const appMessage_t *msg);
    PinSetup getSensor(){return _sensor;}
    PinSetup getActuator(){return _actuator;}
};

#ifdef UNIT_TEST
Shutter *getShutter(int i);
int getSizeOfShutters();
#endif

#endif // __SHUTTER_H__