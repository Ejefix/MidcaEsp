#pragma once
#include <ArduinoJson.h>
#include "gpio_pin.h"
#include "IInputDevice.h"
#include "skeleton.h"

class SwitchMechanics : public IInputDevice, public IExecutor
{
public:
  SwitchMechanics() = delete;
  explicit SwitchMechanics(IGpioPin *gpio_, uint16_t id_);

  InputEvent event() override;
  DeviceType type() override;

protected:
  DeviceResult executeAction(const ScheduledIntent &intent) override;

private:
  IGpioPin *gpio;
  bool status{false};
};

class SwitchButton : public IInputDevice, public IExecutor
{
public:
  SwitchButton() = delete;
  explicit SwitchButton(IGpioPin *gpio_, uint16_t id_);

  InputEvent event() override;
  DeviceType type() override;

protected:
  DeviceResult executeAction(const ScheduledIntent &intent) override;

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