#pragma once

class Wire
{
public:
  Wire();
  ~Wire();

  void begin(uint8_t sda, uint8_t scl, uint32_t freq)
  {

  }
private:

};

Wire::Wire()
{
}

Wire::~Wire()
{
}

class TwoWire
{
protected:

private:
public:
  TwoWire(uint8_t bus_num) {};
  ~TwoWire() {};

  void begin(uint8_t sda, uint8_t scl, uint32_t freq)
  {

  }
};

extern TwoWire Wire;