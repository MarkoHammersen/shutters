#ifndef __ACTUATOR_H__
#define __ACTUATOR_H__

#include "MCP23017.h"

class Actuator
{
private:
    uint32_t _i2cAddr;
    MCP23017 _mcp;

    void _switchOn(MCP23017Pin::Names pin){ _mcp.digitalWrite(pin, HIGH); }
    void _switchOff(MCP23017Pin::Names pin){ _mcp.digitalWrite(pin, LOW); }
    uint32_t _getI2cAddr() { return _i2cAddr; }

public:
    ~Actuator() {}
    Actuator(uint32_t i2cAddr)
    {
        _i2cAddr = i2cAddr;
        _mcp = MCP23017(_i2cAddr);
    }

    void init()
    {
        _mcp.init();

        _mcp.portMode(MCP23017Port::A, 0); // Port A as output
        _mcp.portMode(MCP23017Port::B, 0); // Port B as output

        _mcp.writeRegister(MCP23017Register::GPIO_A, 0x00); // Reset port A
        _mcp.writeRegister(MCP23017Register::GPIO_B, 0x00); // Reset port A
    }    

    void processMsg(const appMessage_t *msg)
    {
        switch (msg->evt)
        {
        case RUN:
            if(_i2cAddr == msg->i2cAddr)
            {
                _switchOn(static_cast<MCP23017Pin::Names>(msg->data));
            }
            /* code */
            break;

        case STOP:
            if(_i2cAddr == msg->i2cAddr)
            {
                _switchOff(static_cast<MCP23017Pin::Names>(msg->data));
            }
            /* code */
            break;

        default:
            break;
        }
    }
};

void initActuators();

#endif // __ACTUATOR_H__
