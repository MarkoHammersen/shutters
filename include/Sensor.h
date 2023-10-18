#pragma once

#include "MCP23017.h"

class Sensor
{
protected:
    MCP23017Name _name;
    MCP23017Pin::Names _up;
    MCP23017Pin::Names _down;

public:
    Sensor(){};
    Sensor(MCP23017Name name, MCP23017Pin::Names up, MCP23017Pin::Names down)
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
    MCP23017Name getMcp23017Name()
    {
        return _name;
    }
};
