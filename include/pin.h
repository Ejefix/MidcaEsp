#ifndef PIN_H
#define PIN_H
#include <set>
#include <skeleton.h>
#include "commandexecutor.h"
#include <ArduinoJson.h>
#include "pindriver.h"
#include "IInputDevice.h"
#include "sensor.h"
#include "DeviceFactory.h"

using PinId = uint32_t;

class PIN : public Skeleton
{
public:
  PIN() = delete;
  PIN(const PIN &) = delete;
  PIN &operator=(const PIN &) = delete;

  explicit PIN(IPinDriver *pin_driver_);

  void load();
  

  void set_status_user(unsigned char pin_info_, bool save_ = true);
  void begin();
  void default_on();
  void save() const;
  unsigned int get_id() const;

  bool add_device(DeviceId idDev, bool saved = true);
  void remove_device(DeviceId idDev);

  void set_brightness(int brightness_);
  bool get_user_on() const;
  void set_user_on(bool status);
  static bool changed_flags;
  void fill_json(JsonArray &arr) const;
  void event(const std::vector<Event> &events);
  enum PinFlags : unsigned char
  {
    FLAG_STATUS_PIN = 1 << 0, // состояние
    FLAG_USER_ON = 1 << 1,    // вручную включён
    FLAG_USER_OFF = 1 << 2,   // вручную выключен
    FLAG_SCRIPT = 1 << 3,     // сценарий активен
    FLAG_SENSOR = 1 << 4,     // движение (PIR)
  };

private:
  static unsigned int id_pin;
  friend CommandExecutor;
  void set_status_pin(bool status);
  void handleEvent(InputEvent ev);
  bool isPinActivation(const unsigned long long &time);

  

  //  |=      <— ставим бит в 1
  //  &= ~    <— сбрасываем бит в 0
  //  &       <— проверяем состояние
  // ^=       <- перевернуть бит
  unsigned char pin_info{FLAG_USER_OFF};
  unsigned long time_delay{};
  const unsigned long time_interval{1000};
  std::set<uint16_t> deviceID{};

  IPinDriver *pin_driver{nullptr};
  uint8_t brightness{100};

  const PinId id;
  bool defaultSetup{false};
  unsigned long counter_default = millis();
  
};






#endif // PIN_H
