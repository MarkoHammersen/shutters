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

    TimerHandle_t _timer;
    uint32_t _runTime;


    Msg const *topHndlr(Msg const *msg);
    Msg const *idleHndlr(Msg const *msg);
    Msg const *stopHndlr(Msg const *msg);
    Msg const *runningHndlr(Msg const *msg);
    Msg const *upHndlr(Msg const *msg);
    Msg const *downHndlr(Msg const *msg);

public:
    Shutter(Window w, uint32_t runTime, PinSetup sensor, PinSetup actuator);
    void startHsm(){ onStart(); };
    void processMsg(const appMessage_t *msg);
    uint32_t getRunTime() { return _runTime;}
    PinSetup getSensor(){return _sensor;}
    PinSetup getActuator(){return _actuator;}
    TimerHandle_t getTimerHandle(){return _timer;}
    Window getWindow(){return _window;}
};

#ifdef UNIT_TEST
Shutter *getShutter(int i);
int getSizeOfShutters();
#endif

#endif // __SHUTTER_H__