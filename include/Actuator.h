#pragma once

#include "MCP23017.h"

class Actuator
{
protected:
    string _up, _down;
    MCP23017 *_mcp;

    int _findPin(string pinName)
    {
        // pinName must be of format "A0" to "A7" or "B0" to "B7"
        if (pinName.length() != 2)
        {
            return -1;
        }
        int pin = static_cast<int>(pinName[1] - '0');
        if ((pin < 0) || (pin > 7))
        {
            return -1;
        }

        switch (pinName[0])
        {
        case 'a':
        case 'A':
            return (MCP23017Pin::Names::GPA0 + pin);
            break;
            
        case 'b':
        case 'B':
            return (MCP23017Pin::Names::GPB0 + pin);
            break;

        default:
            break;
        }
        return -1;
    }

public:
    Actuator(){};
    Actuator(MCP23017 *mcp, string up, string down)
    {
        _mcp = mcp;
        _up = up;
        _down = down;

        _mcp->init();
        _mcp->portMode(MCP23017Port::A, 0); // Port A as output
        _mcp->portMode(MCP23017Port::B, 0); // Port B as output

        _mcp->writeRegister(MCP23017Register::GPIO_A, 0x00); // Reset port A
        _mcp->writeRegister(MCP23017Register::GPIO_B, 0x00); // Reset port B
    }
    string getUp()
    {
        return _up;
    }
    string getDown()
    {
        return _down;
    }

    void switchOn(string pinName)
    {
        int pin = _findPin(pinName);
        if (pin < 0)
        {
            return;
        }
        _mcp->digitalWrite(pin, HIGH);
    }

    void switchOff(string pinName)
    {
        int pin = _findPin(pinName);
        if (pin < 0)
        {
            return;
        }
        _mcp->digitalWrite(pin, LOW);
    }
};