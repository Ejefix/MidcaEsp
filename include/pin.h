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

class PIN :  public IExecutor
{
public:
  PIN() = delete;
  PIN(const PIN &) = delete;
  PIN &operator=(const PIN &) = delete;

  explicit PIN(IPinDriver *pin_driver_);

  void load();
  void save() const;
  uint32_t get_version() const;

  //------------------------------

  //------------------------------
  void begin();

  PinId get_id() const;

 
  void fill_json(JsonArray &arr) const;

protected:
  DeviceResult executeAction(const ScheduledIntent &intent) override;

private:
  static uint16_t id_pin;
  const PinId id;

  bool activPIN{false};

  //  |=      <— ставим бит в 1
  //  &= ~    <— сбрасываем бит в 0
  //  &       <— проверяем состояние
  // ^=       <- перевернуть бит
  // unsigned char pin_info{FLAG_USER_OFF};
  IPinDriver *pin_driver{nullptr};
  uint8_t brightness_to{100}; // конечная яркость для FADE
  uint8_t brightness_from{0}; // начальная яркость для FADE
  double brightness{0};       // текущая яркость для FADE
  double step{0};             // шаг изменения яркости для FADE
  uint32_t timeFADE{};        // время установки яркости от brightness_from до brightness_to
  uint32_t timeStep{};        // время последнего обновления яркости для FADE
  uint32_t version{1};
};

#endif // PIN_H
