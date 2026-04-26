#pragma once
#include <Wire.h>

class PCF8574Device {
public:
  PCF8574Device() = delete;
  PCF8574Device(const PCF8574Device &) = delete;
  PCF8574Device &operator=(const PCF8574Device &) = delete;

  explicit PCF8574Device(TwoWire &I2C1, uint8_t addr);

//  void write(bool forced = false);
 // int set_bit(bool status, uint8_t bit_number);

  uint8_t read();
  int get_bit(uint8_t bit_number);

private:
  unsigned long last_operation{0};
  const unsigned long interval{10};

  TwoWire &I2C;
  const uint8_t address;
  uint8_t buffer{0b11111111};        // кэш для записи
  uint8_t currentState{0b11111111};   // текущее считанное состояние
  uint8_t stableCount{};
};