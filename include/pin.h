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

enum class LockPolicyType
{
  TTL,       // живёт до времени end
  INFINITE,  // пока не вытеснен
  EXTENDABLE // продлевается событиями
};

struct Lock
{
  Lock() = default;
  explicit Lock(uint8_t priority_, IntentSource src, ScheduledIntentID i, LockPolicyType pol, timeMS end);
  LockPolicyType policy{LockPolicyType::TTL};
  timeMS endTime{};
  IntentSource source{IntentSource::IntentDEFAULT};
  uint8_t priority{};
  ScheduledIntentID id{};
};
class PIN : public Skeleton
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

  
  Lock get_active_lock(ActionType type) const;
  Lock get_pending_lock(ActionType type) const;
  
  ExecuteResult executor(const ScheduledIntent &intent, uint8_t priority, LockPolicyType policy, timeMS endTime = 0);

  static bool changed_flags;
  void fill_json(JsonArray &arr) const;

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

  bool check_lock(const Lock &lock) const;
  IPinDriver *pin_driver{nullptr};
  uint8_t brightness_to{0};
  uint8_t brightness_from{100};
  uint8_t brightness{100};
  uint32_t timeFADE{};
  Lock active_power_lock{};  // активный lock для ON/OFF/TOGGLE
  Lock pending_power_lock{}; // предыдущий/вытесненный для power

  Lock active_brightness_lock{};  // активный lock для FADE
  Lock pending_brightness_lock{}; // предыдущий/вытесненный для brightness

  Lock *active{nullptr};
  Lock *pending{nullptr};
};

#endif // PIN_H
