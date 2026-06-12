#include "switch433.h"
#include "globals.h"

SwitchMechanics::SwitchMechanics(IGpioPin *gpio_, uint16_t id_) : IInputDevice(id_), gpio(gpio_)
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

DeviceResult SwitchMechanics::executeAction(const ScheduledIntent &intent)
{
  switch (intent.intent.type)
  {
  case ActionType::CONNECT:
  {
    auto fade = std::get_if<ConnectPayload>(&intent.intent.payload);
    if (!fade)
      return DeviceResult::INVALID_PAYLOAD;
    PinId pinID = fade->obj;
    DeviceId device = intent.intent.targetID;
    if (device_binder->connect(device, pinID))
    {
      return DeviceResult::SUCCESS;
    }
    return DeviceResult::INVALID_PAYLOAD;
  }
  case ActionType::DISCONNECT:
  {
    auto fade = std::get_if<ConnectPayload>(&intent.intent.payload);
    if (!fade)
      return DeviceResult::INVALID_PAYLOAD;
    PinId pinID = fade->obj;
    DeviceId device = intent.intent.targetID;
    device_binder->disconnect(device, pinID);
    return DeviceResult::SUCCESS;
  }
  case ActionType::ERASE:
  {
    DeviceId device = intent.intent.targetID;
    device_binder->disconnect(device);
    return DeviceResult::SUCCESS;
  }
  default:
    return DeviceResult::UNSUPPORTED_ACTION;
  }
}

SwitchButton::SwitchButton(IGpioPin *gpio_, uint16_t id_) : IInputDevice(id_), gpio(gpio_)
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

DeviceResult SwitchButton::executeAction(const ScheduledIntent &intent)
{
  switch (intent.intent.type)
  {
  case ActionType::CONNECT:
  {
    auto fade = std::get_if<ConnectPayload>(&intent.intent.payload);
    if (!fade)
      return DeviceResult::INVALID_PAYLOAD;
    PinId pinID = fade->obj;
    DeviceId device = intent.intent.targetID;
    if (device_binder->connect(device, pinID))
    {
      return DeviceResult::SUCCESS;
    }
    return DeviceResult::INVALID_PAYLOAD;
  }
  case ActionType::DISCONNECT:
  {
    auto fade = std::get_if<ConnectPayload>(&intent.intent.payload);
    if (!fade)
      return DeviceResult::INVALID_PAYLOAD;
    PinId pinID = fade->obj;
    DeviceId device = intent.intent.targetID;
    device_binder->disconnect(device, pinID);
    return DeviceResult::SUCCESS;
  }
  case ActionType::ERASE:
  {
    DeviceId device = intent.intent.targetID;
    device_binder->disconnect(device);
    return DeviceResult::SUCCESS;
  }
  default:
    return DeviceResult::UNSUPPORTED_ACTION;
  }
}
