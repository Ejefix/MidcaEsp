#pragma once
#include "gpio_pin.h"
#include <ArduinoJson.h>
#include "IInputDevice.h"

class SENSOR : public IInputDevice
{
public:
  SENSOR() = delete;
  ~SENSOR() = default;
  explicit SENSOR(IGpioPin *gpio_, unsigned int id_);
  InputEvent event() override;  
  DeviceType type()override;  
  void push_script_time(const std::pair<unsigned long long, unsigned long long> &time);
  void set_interval(const unsigned long &time_off);
  unsigned long get_interval()const ;
  void set_status(bool act);
  unsigned int get_id() const;
  void fill_json(JsonArray &arr) const;
  void load();
  static bool changed_flags;
protected:
  const String arr_name{ "sensor" };
private:
  bool get_activ();
  void save() const;
  bool status{ true };
  bool active{false};
  uint64_t time_activ{};
  uint64_t time_on{};
  unsigned long time_off_{ 3000 };
  IGpioPin *gpio;
  const unsigned int id;
  String path{"/device"};
  unsigned long changed_flags_time{};
  std::pair<uint64_t, uint64_t> script_time{{1}, {1}};
};