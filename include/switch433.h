#pragma once
#include <ArduinoJson.h>
#include "gpio_pin.h"
#include "IInputDevice.h"

class SwitchMechanics : public IInputDevice
{
public:
  SwitchMechanics() = delete;
  explicit SwitchMechanics(IGpioPin *gpio_);

  InputEvent event() override;
  DeviceType type()override;  
private:
  IGpioPin *gpio;
  bool status{false};
};

class SwitchButton : public IInputDevice
{
public:
  SwitchButton() = delete;
  explicit SwitchButton(IGpioPin *gpio_);

  InputEvent event() override;
  DeviceType type()override;
private:
  enum class ButtonState
  {
    Idle,        // кнопка отпущена
    Pressed,     // нажата
    SingleClick, // одиночный клик
    DoubleClick, // двойной клик
    LongPress    // длительное удержание
  };
  IGpioPin *gpio;
  ButtonState state{};
  uint32_t press_time{};
  uint32_t click_time{};
};