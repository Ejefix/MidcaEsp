#pragma once
#include <Adafruit_MCP23X17.h>
class IGpioPin {
public:
  IGpioPin();
  virtual ~IGpioPin() = default;

  //чтение состояния пина
  virtual bool read() = 0; 
 
//запись состояния пина
  virtual void write(bool value) = 0; 
  
//INPUT / OUTPUT / INPUT_PULLUP
  virtual void mode(uint8_t mode) = 0; 
};

class MCP23017Pin : public IGpioPin {
public:
  MCP23017Pin(Adafruit_MCP23X17* mcp_, uint8_t pin_) ;

  bool read() override ;

  void write(bool value) override;

  void mode(uint8_t mode_) override;

private:
  Adafruit_MCP23X17* mcp;
  uint8_t pin;
  int control{};
};