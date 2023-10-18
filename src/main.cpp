#ifdef UNIT_TEST
#include "gtest/gtest.h"
#endif
#include <Arduino.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "esp_log.h"

#include "hsm.hpp"
#include "Actuator.h"
#include "Sensor.h"
#include "IoPortExpander.h"
#include "Shutter.h"

static void sensorInterrupt();

/*
Name	    Input				                    Output
          (MSP23017	Up	Down	Interrupt)	    (MSP23017	Up	Down)
Lina Ost	U36	        B0	B1	    B	            U4	        A6	A5
Lina Süd	U36	        B2	B3	    B	            U4	        A4	A3
Flur	    U36	        B4	B5	    B	            U4	        A2	A1
Wäsche	    U36	        B6	B7	    B	            U4	        A0	B6
Bad Süd	    U36	        A3	A2	    A	            U4	        B5	B4
Bad Nord	U36	        A1	A0	    A	            U4	        B3	B2
Julia West	U1	        B0	B1	    B	            U4	        B1	B0
Julia Ost	U1	        B2	B3	    B	            U37	        A6	A5
Paul Ost	U1	        B4	B5	    B	            U37	        A4	A3
Paul Nord	U1	        B6	B7	    B	            U37	        A2	A1
Büro	    U1	        A7	A6	    A	            U37	        B6	B5
*/

static bool interrupt = false;

static vector<IoPortExpander> sensorIoPortExp = {
    IoPortExpander(I2C_ADDR_SENSOR_U18, MCP23017(I2C_ADDR_SENSOR_U18)),
    IoPortExpander(I2C_ADDR_SENSOR_U35, MCP23017(I2C_ADDR_SENSOR_U35))};
static vector<IoPortExpander> actuatorIoPortExp = {
    IoPortExpander(I2C_ADDR_ACTUATOR_U36, MCP23017(I2C_ADDR_ACTUATOR_U36)),
    IoPortExpander(I2C_ADDR_ACTUATOR_U37, MCP23017(I2C_ADDR_ACTUATOR_U37))};

static vector<Shutter> shutters =
    {
        Shutter("Lina", "Ost", Sensor(I2C_ADDR_SENSOR_U18, MCP23017Pin::GPB0, MCP23017Pin::GPB1), Actuator(I2C_ADDR_ACTUATOR_U36, MCP23017Pin::GPB0, MCP23017Pin::GPB0)),
        // Shutter("Lina", "Sued",     Sensor(Port("U36",  "B2"), Port("U36",  "B3")), Actuator(Port("U4",  "A4"), Port("A4", "A3"))),
        // Shutter("Flur", "Sued",     Sensor(Port("U36",  "B4"), Port("U36",  "B5")), Actuator(Port("U4",  "A2"), Port("A2", "A1"))),
        // Shutter("Waesche", "Sued",  Sensor(Port("U36",  "B6"), Port("U36",  "B7")), Actuator(Port("U4",  "A0"), Port("A0", "B6"))),
        // Shutter("Bad", "Sued",      Sensor(Port("U36",  "A3"), Port("U36",  "A2")), Actuator(Port("U4",  "A0"), Port("B5", "B4"))),
        // Shutter("Bad", "Nord",      Sensor(Port("U36",  "A1"), Port("U36",  "A0")), Actuator(Port("U4",  "B5"), Port("B3", "B2"))),
        // Shutter("Julia", "West",    Sensor(Port("U1",   "B0"), Port("U1",   "B1")), Actuator(Port("U4",  "B3"), Port("B1", "B0"))),
        // Shutter("Julia", "Nord",    Sensor(Port("U1",   "B2"), Port("U1",   "B3")), Actuator(Port("U37", "B1"), Port("A6", "A5"))),
        // Shutter("Paul", "Nord",     Sensor(Port("U1",   "B4"), Port("U1",   "B5")), Actuator(Port("U37", "B1"), Port("A4", "A3"))),
        // Shutter("Paul", "Ost",      Sensor(Port("U1",   "B6"), Port("U1",   "B7")), Actuator(Port("U37", "B1"), Port("A2", "A1"))),
        // Shutter("Buero", "Ost",     Sensor(Port("U1",   "A7"), Port("U1",   "A6")), Actuator(Port("U37", "B1"), Port("B6", "B5")))
};

static void sensorInterrupt()
{
  interrupt = true;
}

void setup()
{
  Wire.begin(I2C_SDA, I2C_SCL, I2C_FRQ);

  for (IoPortExpander i : sensorIoPortExp)
  {
    i.initSensors();
  }

  for (IoPortExpander i : actuatorIoPortExp)
  {
    i.initActuators();
  }

  pinMode(SENSOR_U18_INTA, INPUT_PULLUP);
  pinMode(SENSOR_U18_INTB, INPUT_PULLUP);
  pinMode(SENSOR_U35_INTA, INPUT_PULLUP);
  pinMode(SENSOR_U35_INTB, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SENSOR_U18_INTA), sensorInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_U18_INTB), sensorInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_U35_INTA), sensorInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_U35_INTB), sensorInterrupt, FALLING);

  for (Shutter s : shutters)
  {
    s.startHsm();
  }
}

void loop()
{
  if (interrupt)
  {
    noInterrupts();
    for (IoPortExpander i : sensorIoPortExp)
    {
      i.interruptedBy();
    }
    interrupt = false;
    interrupts();

    for (Shutter i : shutters)
    {
      i.processSensorEvents();
    }
  }

  // process timeouts
  for (Shutter i : shutters)
  {
    i.processSensorDebouncing();
  }
}

uint8_t sensorGetSinglePinEvent(uint32_t i2cAddr, MCP23017Pin::Names pin)
{
  for (IoPortExpander i : sensorIoPortExp)
  {
    if (i.getI2cAddr() == i2cAddr)
    {
      uint8_t port;
      if (pin < MCP23017Pin::Names::GPB0)
      {
        port = i.getInterruptedPort(MCP23017Port::A);
        if ((port & (1 << pin)) > 0)
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
      else
      {
        port = i.getInterruptedPort(MCP23017Port::B);
          if((port & (1 << (pin - MCP23017Pin::Names::GPB0))) > 0)
          {
          return 1;
          }
          else
          {
          return 0;
          }
      }
    }
  }

  assert(false); // never get here, due to invalid i2c address
}

void actuatorCmd(uint32_t i2cAddr, MCP23017Pin::Names pin, uint8_t state)
{
  for (IoPortExpander i : actuatorIoPortExp)
  {
    if (i.getI2cAddr() == i2cAddr)
    {
      if (HIGH == state)
      {
          i.switchOn(pin);
      }
      else
      {
          i.switchOff(pin);
      }
      break;
    }
  }
}
