#include "pcfdevice.h"
#include <Arduino.h>


PCF8574Device::PCF8574Device(TwoWire &I2C1, uint8_t addr)
  : I2C(I2C1), address(addr) {
}
/*
void PCF8574Device::write(bool forced) {
  if (buffer == currentState && !forced) return;
  unsigned long now = millis();
  if (now - last_operation < interval) {
    return;
  } else {
    last_operation = now;
  }
 
  I2C.beginTransmission(address);
  I2C.write(buffer);
  uint8_t err = I2C.endTransmission();
  if (err != 0) {
    Serial.print("[PCF WRITE ERROR] Addr 0x");
    Serial.print(address, HEX);
    Serial.print(" | Code: ");
    Serial.println(err);
  } else {
    currentState = buffer;
  }
}

int PCF8574Device::set_bit(bool status, uint8_t bit_number) {
  if (bit_number > 7) return -1;
  bitWrite(buffer, bit_number, status);
  return 0;
}*/
uint8_t PCF8574Device::read() {
  unsigned long now = millis();
  if (now - last_operation < interval) {
    return currentState;
  } else {
    last_operation = now;
  }
  I2C.requestFrom(address, (uint8_t)1);  // запрос 1 байта у PCF
  if (I2C.available()) {                 // если байт пришёл
    uint8_t buffer_;
    buffer_ = I2C.read();  // сохраняем в кэш
    if (buffer_ == buffer) {
      ++stableCount;
      if (stableCount > 2)
        {
          currentState = buffer;
        }
    } else {
      buffer = buffer_;
      stableCount = 0;
    }
  }
  return currentState;
}


int PCF8574Device::get_bit(uint8_t bit_number) {
  if (bit_number > 7) return -1;  // защита от мусорного номера
  return bitRead(currentState, bit_number);
}
