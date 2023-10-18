#pragma once

#include <stdint.h>

//Interrupt Modes
#define RISING    0x01
#define FALLING   0x02
#define CHANGE    0x03
#define ONLOW     0x04
#define ONHIGH    0x05
#define ONLOW_WE  0x0C
#define ONHIGH_WE 0x0D

#define INPUT             0x01
#define OUTPUT            0x03 
#define PULLUP            0x04
#define INPUT_PULLUP      0x05
#define PULLDOWN          0x08
#define INPUT_PULLDOWN    0x09
#define OPEN_DRAIN        0x10
#define OUTPUT_OPEN_DRAIN 0x12
#define ANALOG            0xC0

#define HIGH 1
#define LOW 1

#define I2C_SDA  1
#define I2C_SCL  2
#define I2C_FRQ  100000
#define I2C_ADDR_SENSOR_U18  0x20
#define I2C_ADDR_SENSOR_U35  0x21
#define I2C_ADDR_ACTUATOR_U36  0x22
#define I2C_ADDR_ACTUATOR_U37  0x23
#define SENSOR_U18_INTA  61
#define SENSOR_U18_INTB  62
#define SENSOR_U35_INTA  63
#define SENSOR_U35_INTB  64
#define HSM_DEBOUNCE_TIME  500

#define digitalPinToInterrupt(p)    (((p)<40)?(p):-1)

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

unsigned long millis();
void pinMode(uint8_t pin, uint8_t mode);
void noInterrupts();
void interrupts();
void attachInterrupt(uint8_t pin, void* intRoutine, int mode);