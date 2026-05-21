#include "gpio_pin.h"
#include <Arduino.h>




IGpioPin::IGpioPin() 
{
   
}



MCP23017Pin::MCP23017Pin(Adafruit_MCP23X17 *mcp_, uint8_t pin_): mcp(mcp_), pin(pin_)
{

}


bool MCP23017Pin::read()
{   
     if(mcp->digitalRead(pin)) 
     {
       ++control;
     } 
     else
     {
      control = 0;
     }
     if(control > 2)
     {
      control = 2;
      return true;
     } 
     else
     {
      return false;
     } 
}

void MCP23017Pin::write(bool value)
{
  mcp->digitalWrite(pin, value);
}

void MCP23017Pin::mode(uint8_t mode_)
{
   mcp->pinMode(pin, mode_);
}
