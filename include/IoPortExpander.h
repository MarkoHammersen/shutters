#ifndef __IO_EXPANDER_H__
#define __IO_EXPANDER_H__

#include "DeviceNames.h"
#include "MCP23017.h"
#include "Wire.h"

class IoPortExpander
{
private:
    MCP23017Name _name;
    MCP23017 _mcp;
    uint8_t _iPortA;
    uint8_t _iPortB;
public:
    
    IoPortExpander(MCP23017Name name, MCP23017 mcp);
    ~IoPortExpander();

    void switchOff(MCP23017Pin::Names pin);
    void switchOn(MCP23017Pin::Names pin);
    void initSensors();
    void initActuators();
    void interruptedBy();
    uint8_t getInterruptedPort(MCP23017Port port);
    MCP23017Name getName();
};

void IoPortExpander::switchOn(MCP23017Pin::Names pin)
{
    _mcp.digitalWrite(pin, HIGH);
}

void IoPortExpander::switchOff(MCP23017Pin::Names pin)
{
    _mcp.digitalWrite(pin, LOW);
}

MCP23017Name IoPortExpander::getName()
{
    return _name;
}

IoPortExpander::IoPortExpander(MCP23017Name name, MCP23017 mcp)
{
    _name = name;
    _mcp = mcp;
}

IoPortExpander::~IoPortExpander()
{
}

void IoPortExpander::initSensors()
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

void IoPortExpander::initActuators()
{
    _mcp.init();

    _mcp.portMode(MCP23017Port::A, 0);          //Port A as output
    _mcp.portMode(MCP23017Port::B, 0);          //Port B as output

    _mcp.writeRegister(MCP23017Register::GPIO_A, 0x00);  //Reset port A 
    _mcp.writeRegister(MCP23017Register::GPIO_B, 0x00);  //Reset port A 
}

void IoPortExpander::interruptedBy()
{
    _mcp.interruptedBy(_iPortA, _iPortB);
}

uint8_t IoPortExpander::getInterruptedPort(MCP23017Port port)
{
    return (MCP23017Port::A == port) ? _iPortA: _iPortB;
}

#endif // __IO_EXPANDER_H__