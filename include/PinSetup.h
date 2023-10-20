#ifndef __PINSETUP_H__
#define __PINSETUP_H__

#include "MCP23017.h"

class PinSetup
{
protected:
    uint8_t _i2cAddr;
    int _up;
    int _down;

public:
    PinSetup(){};
    PinSetup(uint32_t i2cAddr, int up, int down)
    {
        _i2cAddr = i2cAddr;
        _up = up;
        _down = down;
    }
    int getUp()
    {
        return _up;
    }
    int getDown()
    {
        return _down;
    }
    uint8_t getI2cAddr()
    {
        return _i2cAddr;
    }
};

#endif // __PINSETUP_H__