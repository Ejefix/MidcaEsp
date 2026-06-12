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
#include "scenario_intent_system.h"

using PinId = uint16_t;

class PIN : public Skeleton, public IExecutor
{
public:
  PIN() = delete;
  PIN(const PIN &) = delete;
  PIN &operator=(const PIN &) = delete;

  explicit PIN(IPinDriver *pin_driver_);

  void load();
  void save() const;
  //------------------------------

  //------------------------------
  void begin();

  PinId get_id() const;

  static bool changed_flags;
  void fill_json(JsonArray &arr) const;

protected:
  DeviceResult executeAction(const ScheduledIntent &intent) override;

private:
  static uint16_t id_pin;
  const PinId id;
  friend CommandExecutor;

  bool activPIN{false};

  //  |=      <— ставим бит в 1
  //  &= ~    <— сбрасываем бит в 0
  //  &       <— проверяем состояние
  // ^=       <- перевернуть бит
  // unsigned char pin_info{FLAG_USER_OFF};
  IPinDriver *pin_driver{nullptr};
  uint8_t brightness_to{0};
  uint8_t brightness_from{100};
  uint8_t brightness{100};
  uint32_t timeFADE{};
};

#endif // PIN_H
