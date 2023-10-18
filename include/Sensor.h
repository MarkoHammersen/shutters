#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "MCP23017.h"

class Sensor
{
protected:
    uint32_t _i2cAddr;
    MCP23017Pin::Names _up;
    MCP23017Pin::Names _down;

public:
    Sensor(){};
    Sensor(uint32_t i2cAddr, MCP23017Pin::Names up, MCP23017Pin::Names down)
    {
        _i2cAddr = i2cAddr;
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
    uint32_t getI2cAddr()
    {
        return _i2cAddr;
    }
};

#endif // __SENSOR_H__