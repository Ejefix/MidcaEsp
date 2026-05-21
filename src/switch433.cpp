#include "switch433.h"

SwitchMechanics::SwitchMechanics(IGpioPin *gpio_)
    : gpio(gpio_)
{
  status = gpio->read();
}
InputEvent SwitchMechanics::event()
{
  if (!gpio)
  {
    return InputEvent::NoEventErr;
  }
  bool check = gpio->read();
  if (status != check)
  {
    status = check;
    return InputEvent::Toggle;
  }
  else
  {
    return InputEvent::NoEvent;
  }
}

DeviceType SwitchMechanics::type()
{
    return DeviceType::Switch;
}

SwitchButton::SwitchButton(IGpioPin *gpio_) : gpio(gpio_)
{
}

InputEvent SwitchButton::event()
{
  if (!gpio)
    return InputEvent::NoEventErr;

  bool check = gpio->read();
  uint32_t now = millis();

  if (check && state == ButtonState::Idle)
  {
    press_time = now;
    state = ButtonState::Pressed;
    return InputEvent::NoEvent;
  }
  if ((state == ButtonState::Pressed || state == ButtonState::DoubleClick) && check && now - press_time > 1500)
  {
    state = ButtonState::LongPress;
    return InputEvent::LongPress;
  }

  if (state == ButtonState::Pressed && !check)
  {
    click_time = now;
    state = ButtonState::SingleClick;
    return InputEvent::NoEvent;
  }
  if (state == ButtonState::SingleClick && !check && now - click_time > 400)
  {
    state = ButtonState::Idle;
    return InputEvent::Toggle;
  }
  if (state == ButtonState::SingleClick && check)
  {
    press_time = now;
    state = ButtonState::DoubleClick;
    return InputEvent::NoEvent;
  }
  if (state == ButtonState::DoubleClick && !check)
  {
    state = ButtonState::Idle;
    return InputEvent::DoubleClick;
  }
  if (state == ButtonState::LongPress && !check)
  {
    state = ButtonState::Idle;
  }
  return InputEvent::NoEvent;
}

DeviceType SwitchButton::type()
{
    return DeviceType::Button;
}
