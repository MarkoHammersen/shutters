
#include <Arduino.h>
#include <Wire.h>

#if 1
extern void initShutters();
extern void initActuators();  
extern void initSensorTask();

void setup()
{
  ESP_LOGI("main", "setup ENTRY");
  Wire.begin(I2C_SDA, I2C_SCL, I2C_FRQ);
//  initActuators();  
  delay(20); // allow actuators some time to turn off all outputs
  //initShutters();  
  initSensorTask();
  ESP_LOGI("main", "setup EXITY");
}

void loop()
{

}

#else
#include <MCP23017.h>

#define INT_PIN_U18_A (SENSOR_U18_INTA)
#define INT_PIN_U18_B (SENSOR_U18_INTB)
#define INT_PIN_U2_A (SENSOR_U2_INTA)
#define INT_PIN_U2_B (SENSOR_U2_INTB)

MCP23017 mcpU18 = MCP23017(I2C_ADDR_SENSOR_U18);
MCP23017 mcpU2 = MCP23017(I2C_ADDR_SENSOR_U2);

volatile bool interruptedU18 = false;
volatile bool interruptedU2 = false;

void userInput_U18_A()
{
  interruptedU18 = true;
}

void userInput_U18_B()
{
  interruptedU18 = true;
}

void userInput_U2_A()
{
  interruptedU2 = true;
}

void userInput_U2_B()
{
  interruptedU2 = true;
}

void setup()
{
  Wire.begin();
  Serial.begin(115200);

  mcpU18.init();
  mcpU18.portMode(MCP23017Port::A, 0b01111111); // Port A as input
  mcpU18.portMode(MCP23017Port::B, 0b01111111); // Port B as input
  mcpU18.interruptMode(MCP23017InterruptMode::Separated);
  mcpU18.interrupt(MCP23017Port::A, FALLING);
  mcpU18.interrupt(MCP23017Port::B, FALLING);
  mcpU18.writeRegister(MCP23017Register::IPOL_A, 0x00);
  mcpU18.writeRegister(MCP23017Register::IPOL_B, 0x00);
  mcpU18.writeRegister(MCP23017Register::GPIO_A, 0x00);
  mcpU18.writeRegister(MCP23017Register::GPIO_B, 0x00);
  mcpU18.clearInterrupts();

  pinMode(INT_PIN_U18_A, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN_U18_A), userInput_U18_A, FALLING);
  pinMode(INT_PIN_U18_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN_U18_B), userInput_U18_B, FALLING);

  mcpU2.init();
  mcpU2.portMode(MCP23017Port::A, 0b01111111); // Port A as input
  mcpU2.portMode(MCP23017Port::B, 0b01111111); // Port B as input
  mcpU2.interruptMode(MCP23017InterruptMode::Separated);
  mcpU2.interrupt(MCP23017Port::A, FALLING);
  mcpU2.interrupt(MCP23017Port::B, FALLING);
  mcpU2.writeRegister(MCP23017Register::IPOL_A, 0x00);
  mcpU2.writeRegister(MCP23017Register::IPOL_B, 0x00);
  mcpU2.writeRegister(MCP23017Register::GPIO_A, 0x00);
  mcpU2.writeRegister(MCP23017Register::GPIO_B, 0x00);
  mcpU2.clearInterrupts();

  pinMode(INT_PIN_U2_A, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN_U2_A), userInput_U2_A, FALLING);
  pinMode(INT_PIN_U2_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN_U2_B), userInput_U2_B, FALLING);
}

void loop()
{
  uint8_t a, b;
  uint8_t captureA, captureB;

  if (!interruptedU18)
  {
    mcpU18.clearInterrupts();
  }

  if (!interruptedU2)
  {
    mcpU2.clearInterrupts();
  }

  if (!interruptedU18 && !interruptedU2)
  {
    return;
  }

  // debouncing
  // delay(100);

  if (interruptedU18)
  {
    interruptedU18 = false;
    mcpU18.interruptedBy(a, b);
    // this is the state of the port the moment the interrupt was triggered
    mcpU18.clearInterrupts(captureA, captureB);
    // this is the state of the B port right now, after the delay to act as debouncing
    (void)mcpU18.readPort(MCP23017Port::B);
    (void)mcpU18.readPort(MCP23017Port::A);

    if (a > 0)
      log_i("A: %02x", a);
    if (b > 0)
      log_i("B: %02x", b);
  }

  if (interruptedU2)
  {
    interruptedU2 = false;
    mcpU2.interruptedBy(a, b);
    // this is the state of the port the moment the interrupt was triggered
    mcpU2.clearInterrupts(captureA, captureB);
    // this is the state of the B port right now, after the delay to act as debouncing
    (void)mcpU2.readPort(MCP23017Port::B);
    (void)mcpU2.readPort(MCP23017Port::A);

    if (a > 0)
      log_i("A: %02x", a);
    if (b > 0)
      log_i("B: %02x", b);
  }
}
#endif