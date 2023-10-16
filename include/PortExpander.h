#pragma once

#include "MCP23017.h"
#include "Wire.h"

struct PORT_EXPANDER_TYPE
{
    enum Names
    {
        SENSOR,
        ACTUATOR
    };
};

class PortExpander
{
private:
    string name;
    MCP23017 mcp;
    int findPin(string pinName);

public:
    PortExpander(PORT_EXPANDER_TYPE type, string name, MCP23017 mcp);
    ~PortExpander();

    void switchOff(string pinName);
    void switchOn(string pinName);
};

int PortExpander::findPin(string pinName)
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
        return (MCP23017Pin::GPA0 + pin);

        break;
    case 'b':
    case 'B':
        return (MCP23017Pin::GPB0 + pin);
        break;

    default:
        break;
    }
    return -1;
}

void PortExpander::switchOn(string pinName)
{
    int pin = findPin(pinName);
    if (pin < 0)
    {
        return;
    }
    mcp.digitalWrite(pin, HIGH);
}

void PortExpander::switchOff(string pinName)
{
    int pin = findPin(pinName);
    if (pin < 0)
    {
        return;
    }
    mcp.digitalWrite(pin, LOW);
}

PortExpander::PortExpander(PORT_EXPANDER_TYPE type, string name, MCP23017 &mcp)
{
    Wire.begin(I2C_SDA, I2C_SCL, I2C_FRQ);

    if (PORT_EXPANDER_TYPE::ACTUATOR == type)
    {
        mcp.init();
        mcp.portMode(MCP23017Port::A, 0); // Port A as output
        mcp.portMode(MCP23017Port::B, 0); // Port B as output

        mcp.writeRegister(MCP23017Register::GPIO_A, 0x00); // Reset port A
        mcp.writeRegister(MCP23017Register::GPIO_B, 0x00); // Reset port B
    }
    else
    {
        return;
    }

    name = name;
    mcp = mcp;
}

PortExpander::PortExpander(PORT_EXPANDER_TYPE type, string name, MCP23017 &mcp, uint8_t intPinA, , uint8_t intPinB)
{
    Wire.begin(I2C_SDA, I2C_SCL, I2C_FRQ);

    if (PORT_EXPANDER_TYPE::SENSOR == type)
    {
        mcp.init();
        mcp.portMode(MCP23017Port::A, 0b11111111); // Port A as input
        mcp.portMode(MCP23017Port::B, 0b11111111); // Port B as input

        mcp.interruptMode(MCP23017InterruptMode::Separated);
        mcp.interrupt(MCP23017Port::A, FALLING);
        mcp.interrupt(MCP23017Port::B, FALLING);

        mcp.writeRegister(MCP23017Register::IPOL_A, 0x00);
        mcp.writeRegister(MCP23017Register::IPOL_B, 0x00);

        mcp.writeRegister(MCP23017Register::GPIO_A, 0x00);
        mcp.writeRegister(MCP23017Register::GPIO_B, 0x00);

        mcp.clearInterrupts();

        pinMode(intPinA, INPUT_PULLUP);
        pinMode(intPinB, INPUT_PULLUP);

        attachInterrupt(digitalPinToInterrupt(intPinA), userInputA, FALLING);
        attachInterrupt(digitalPinToInterrupt(intPinB), userInputB, FALLING);
    }
    else
    {
        return;
    }

    name = name;
    mcp = mcp;
}

PortExpander::~PortExpander()
{
}
