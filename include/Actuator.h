#ifndef __ACTUATOR_H__
#define __ACTUATOR_H__

#include "DeviceNames.h"
#include "MCP23017.h"

class Actuator
{
protected:
    MCP23017Pin::Names _up, _down;
    MCP23017Name _name;
public:
    Actuator(){};
    Actuator(MCP23017Name name, MCP23017Pin::Names up, MCP23017Pin::Names down)
    {
        _name = name;
        _up = up;
        _down = down;
    }
    MCP23017Pin::Names getUp()
    {
        return _up;
    }
    MCP23017Pin::Names getDown()
    {
        return _down;
    }
};

#endif // __ACTUATOR_H__