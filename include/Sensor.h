#ifndef __SENSOR_H_
#define __SENSOR_H_

class Sensor
{
private:
    uint32_t _i2cAddr;
    MCP23017 _mcp;
    uint8_t _iPortA;
    uint8_t _iPortB;

public:
    Sensor(uint8_t i2cAddr)
    {
        _mcp = MCP23017(i2cAddr);
        _i2cAddr = i2cAddr;
        _iPortA = 0;
        _iPortB = 0;
    };
    ~Sensor(){};

    uint8_t getI2cAddr(){return _i2cAddr;}

    uint8_t getInterruptPort(MCP23017Port port)
    {
        return port == MCP23017Port::A ? _iPortA : _iPortB;
    }

    void clearInterruptPorts()
    {
        _iPortA = 0;
        _iPortB = 0;
    }

    void init()
    {
    _mcp.init();
    _mcp.portMode(MCP23017Port::A, 0b11111111); // Port A as input
    _mcp.portMode(MCP23017Port::B, 0b11111111); // Port B as input

    _mcp.interruptMode(MCP23017InterruptMode::Separated);
    _mcp.interrupt(MCP23017Port::A, FALLING);
    _mcp.interrupt(MCP23017Port::B, FALLING);

    _mcp.writeRegister(MCP23017Register::IPOL_A, 0x00);
    _mcp.writeRegister(MCP23017Register::IPOL_B, 0x00);

    _mcp.writeRegister(MCP23017Register::GPIO_A, 0x00);
    _mcp.writeRegister(MCP23017Register::GPIO_B, 0x00);

    _mcp.clearInterrupts();
    }

    void interruptedBy()
    {
        _mcp.interruptedBy(_iPortA, _iPortB);
    }
};

#endif // __SENSOR_H_