#include <Arduino.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "DeviceNames.h"
#include "hsm.hpp"
#include "Actuator.h"
#include "Sensor.h"
#include "IoPortExpander.h"
#include "Shutter.h"

using namespace std;
#define TAG "shutter"

static void sensorInterrupt();
static void processSensorEvent(MCP23017Name mcpName, MCP23017Port portNumber, uint8_t portValue);

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
static vector<IoPortExpander> sensorIoPortExp ={
  IoPortExpander(MCP23017Name::U18, MCP23017(I2C_ADDR_SENSOR_U18)),
  IoPortExpander(MCP23017Name::U35, MCP23017(I2C_ADDR_SENSOR_U35))
};
static vector<IoPortExpander> actuatorIoPortExp ={
  IoPortExpander(MCP23017Name::U36, MCP23017(I2C_ADDR_ACTUATOR_U36)),
  IoPortExpander(MCP23017Name::U37, MCP23017(I2C_ADDR_ACTUATOR_U37))
};

static vector<Shutter> shutters =
    {
        Shutter("Lina", "Ost", Sensor(MCP23017Name::U18, MCP23017Pin::GPB0, MCP23017Pin::GPB1), Actuator(MCP23017Name::U36, MCP23017Pin::GPB0, MCP23017Pin::GPB0)),
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

static void processSensorEvent(MCP23017Name mcpName, MCP23017Port portNumber, uint8_t portValue)
{
  int offset = (MCP23017Port::A == portNumber)  ? MCP23017Pin::GPA0 : MCP23017Pin::GPB0;
  for (int i = 0; i < 8; i++)
    {
      if ((portValue & (1u << i)) > 0)
      {
        for (Shutter s : shutters)
        {
          if(mcpName != s.sensor.getMcp23017Name())
          {
            continue;
          }
        
          if (s.sensor.getUp() == static_cast<MCP23017Pin::Names>(i) + offset)
          {
            struct Msg msg = {SENSOR_UP_EVT};
            s.onEvent(&msg);
            switch(s.getShutterCmd())
            {
              case ShutterCmd::SWITCH_UP_ON:
                for(IoPortExpander io: actuatorIoPortExp)
                {
                  
                }
              break;
            }

          }
          else if (s.sensor.getDown() == static_cast<MCP23017Pin::Names>(i) + offset)
          {
            struct Msg msg = {SENSOR_DOWN_EVT};
            s.onEvent(&msg);
          }
          else
          {
          }
        }
      }
    }
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
    s.onStart();
  }
}


void loop()
{
  // do nothing here, as software runs in dedicated tasks
  uint8_t u18PortA;
  uint8_t u18PortB;
  uint8_t u36PortA;
  uint8_t u36PortB;

  if (interrupt)
  {
    noInterrupts();
    for(IoPortExpander i: sensorIoPortExp)
    {
      i.interruptedBy();
    }
    interrupt = false;
    interrupts();

    for(IoPortExpander i: sensorIoPortExp)
    {
      //processSensorEvent(i.getName(), MCP23017Port::A,  i.getInterruptedPort(MCP23017Port::A));
      //processSensorEvent(i.getName(), MCP23017Port::B,  i.getInterruptedPort(MCP23017Port::B));
    }
  }
}