#ifndef __SENSOR_H_
#define __SENSOR_H_

class Sensor
{
private:
    uint32_t _i2cAddr;
    MCP23017 _mcp;
    uint8_t _iPortA, _iPortB;
    Timer<> _timer;
    const char *_name;
    volatile bool _interrupted;

public:
    Sensor(const char *name, uint8_t i2cAddr, TwoWire &bus)
    {
        _name = name;
        _mcp = MCP23017(i2cAddr, bus);
        _i2cAddr = i2cAddr;
        _iPortA = 0;
        _iPortB = 0;
        _interrupted = false;
        _timer = timer_create_default();
    };
    ~Sensor(){};

    uint8_t getI2cAddr() { return _i2cAddr; }
    const char *getName() { return _name; }
    void interruptCallback(void){ _interrupted = true;}
    void startTimer(Timer<>::handler_t h)
    {
        _timer.in(MCP23017_INTERRUPT_DEBOUNCE_TIME, h, NULL);
    }

    bool isInterrupted()
    { 
        if(_interrupted)
        {
            _interrupted = false;

            _iPortA = 0;
            _iPortB = 0;
            _mcp.interruptedBy(_iPortA, _iPortB);

            if(_iPortA > 0)
            {
                // clear interrupts
                _mcp.readRegister(MCP23017Register::INTCAP_A);
                // we need to read GPIOAB to clear the interrupt actually.
                _mcp.readPort(MCP23017Port::A);
            }

            if(_iPortB > 0)
            {
                // clear interrupts
                _mcp.readRegister(MCP23017Register::INTCAP_B);
                // we need to read GPIOAB to clear the interrupt actually.
                _mcp.readPort(MCP23017Port::B);
            }

            return true;
        }
        return false;
    }

    uint8_t getInterruptPort(MCP23017Port port)
    {
        return port == MCP23017Port::A ? _iPortA : _iPortB;
    }

    void clearInterrupts()
    {
        _mcp.clearInterrupts();
    }

    uint8_t read()
    {
        return _mcp.read();
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

    void interruptedBy()
    {
        _iPortA = 0;
        _iPortB = 0;
        _mcp.interruptedBy(_iPortA, _iPortB);
    }
};

#endif // __SENSOR_H_