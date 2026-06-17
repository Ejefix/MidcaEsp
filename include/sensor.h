#pragma once
#include "gpio_pin.h"
#include <ArduinoJson.h>
#include "IInputDevice.h"
#include <skeleton.h>

class SENSOR : public IInputDevice, public IExecutor
{
public:
  SENSOR() = delete;
  ~SENSOR() = default;
  explicit SENSOR(IGpioPin *gpio_, uint16_t id_);
  InputEvent event() override;
  DeviceType type() override;
  
  unsigned long get_interval() const;
  void fill_json(JsonArray &arr) const;
  void load();
  

protected:
  const String arr_name{"sensor"};
  DeviceResult executeAction(const ScheduledIntent &intent) override;

private:
  bool get_activ();
  void save() const;
  bool status{true};
  bool active{false};
  uint64_t time_activ{};
  uint32_t time_off{3000};
  IGpioPin *gpio;
  String path{"/device"};
};