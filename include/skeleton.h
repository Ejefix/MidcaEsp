#ifndef SKELETON_H
#define SKELETON_H
#include <Arduino.h>
#include <array>
#include "scenario_intent_system.h"

class Skeleton
{
public:
  Skeleton();

  enum
  {
    accepted, start , finish,

    idESP, idUSER, idSer, 

    intent, ping_pong,

    snapshot_device, snapshot_store, snapshot_intent, 
    snapshot_pins,  snapshot_connect, 

    end
  };
  static const std::array<String, Skeleton::end> commands;
  static String id;
};

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

enum class DeviceResult
{
  SUCCESS,            // выполнено
  INVALID_PAYLOAD,    // payload не соответствует ActionType
  DRIVER_NOT_FOUND,   // отсутствует драйвер
  UNSUPPORTED_ACTION, // действие не поддерживается
};

class IExecutor
{
public:
  
  virtual ~IExecutor() = default;

  ExecuteResult execute(const ScheduledIntent &intent, uint8_t priority, LockPolicyType policy, timeMS endTime);
  Lock get_active_lock(ActionType type) const;
  Lock get_pending_lock(ActionType type) const;

protected:
  virtual DeviceResult executeAction(const ScheduledIntent &intent) = 0;

private:
  Lock active_power_lock{};  // активный lock для ON/OFF/TOGGLE
  Lock pending_power_lock{}; // предыдущий/вытесненный для power

  Lock active_brightness_lock{};  // активный lock для FADE
  Lock pending_brightness_lock{}; // предыдущий/вытесненный для brightness

  Lock *active{&active_power_lock};
  Lock *pending{&pending_power_lock};
  bool check_lock(const Lock &lock) const;
};
#endif // SKELETON_H
