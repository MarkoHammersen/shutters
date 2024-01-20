#ifndef __SENSOR_H_
#define __SENSOR_H_

class Sensor
{
private:
    uint32_t _i2cAddr;
    MCP23017 _mcp;
    const char *_name;
    uint8_t _pinIntPortA, _pinIntPortB;
    uint16_t _prevGpioAB;

public:
    Sensor(const char *name, uint8_t i2cAddr, uint8_t pinIntPortA, uint8_t pinIntPortB, TwoWire &bus)
    {
        _name = name;
        _mcp = MCP23017(i2cAddr, bus);
        _i2cAddr = i2cAddr;
        _pinIntPortA = pinIntPortA;
        _pinIntPortB = pinIntPortB;
        _prevGpioAB = 0;
    };
    ~Sensor(){};

    uint8_t getI2cAddr()
    {
        return _i2cAddr;
    }

    const char *getName()
    {
        return _name;
    }

    uint8_t getPinIntPort(MCP23017Port port)
    {
        return port == MCP23017Port::A ? _pinIntPortA: _pinIntPortB;
    }

    uint16_t getGpioAB(uint16_t *gpioAB)
    {
        _mcp.clearInterrupts();
        uint16_t val = _mcp.read();
        
        val = (0xFFFF - val) & 0x7f7f;
        if (val != _prevGpioAB)
        {
            _prevGpioAB = val;
            *gpioAB = val;

            return true;
        }

        *gpioAB = 0;
        return false; // no change
    }

    void init()
    {
        _mcp.init();

        // configure inputs
        // GPA7 & GPB7 Cannot Be Used as Inputs In MCP23017
        // see: https://microchip.my.site.com/s/article/GPA7---GPB7-Cannot-Be-Used-as-Inputs-In-MCP23017
        _mcp.portMode(MCP23017Port::A, 0b01111111); // Port A as input
        _mcp.portMode(MCP23017Port::B, 0b01111111); // Port B as input

        _mcp.interruptMode(MCP23017InterruptMode::Separated);
        _mcp.interrupt(MCP23017Port::A, FALLING);
        _mcp.interrupt(MCP23017Port::B, FALLING);

        _mcp.writeRegister(MCP23017Register::IPOL_A, 0x00);
        _mcp.writeRegister(MCP23017Register::IPOL_B, 0x00);
        // reset:
        _mcp.writeRegister(MCP23017Register::GPIO_A, 0x00);
        _mcp.writeRegister(MCP23017Register::GPIO_B, 0x00);

        _mcp.clearInterrupts();
    }
};

#endif // __SENSOR_H_