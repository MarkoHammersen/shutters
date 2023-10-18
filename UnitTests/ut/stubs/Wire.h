#pragma once

#include <stdint.h>

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

  void beginTransmission(uint8_t address)
  {
    
  }

  uint8_t endTransmission(void)
  {
    return 0;
  }

  int read(void)
  {
    return 0;
  }

  uint8_t requestFrom(uint8_t address, uint8_t size)
  {
    return 0;
  }

  size_t write(uint8_t byte)
  {
    return 1;
  }
  size_t write(const uint8_t* data, size_t size)
  {
    return size;
  }
};

extern TwoWire Wire;