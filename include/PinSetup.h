#ifndef __PINSETUP_H__
#define __PINSETUP_H__

#include "MCP23017.h"

class PinSetup
{
protected:
    uint8_t _i2cAddr;
    uint16_t _up;
    uint16_t _down;

public:
    PinSetup(){};
    PinSetup(uint32_t i2cAddr, uint16_t up, uint16_t down)
    {
        _i2cAddr = i2cAddr;
        _up = up;
        _down = down;
    }
    uint16_t getUp()
    {
        return _up;
    }
    uint16_t getDown()
    {
        return _down;
    }
    uint8_t getI2cAddr()
    {
        return _i2cAddr;
    }
};

#endif // __PINSETUP_H__