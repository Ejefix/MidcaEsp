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
    pin1,
    pin2,
    pin3,
    pin4,
    pin5,
    pin6,
    pin7,
    pin8,
    pin9,
    pin10,
    pin11,
    pin12,
    pin13,
    pin14,
    pin15,
    pin16,
    pin17,
    pin18,
    pin19,
    pin20,
    pin21,
    pin22,
    pin23,
    pin24,
    pin25,
    pin26,
    pin27,
    pin28,
    pin29,
    pin30,
    pin31,
    pin32,
    pin33,
    pin34,
    pin35,

    idESP,
    idUSER,
    idSer,

    accepted,
    start,
    separator,
    finish,

    on,
    off,
    time_on,
    time_off,
    user_on,
    user_off,

    script_on,
    script_off,
    brightness,

    pir,
    push,
    remove,
    sm,

    status,
    status_full,

    end
  };
  const std::array<String, Skeleton::end> commands{
      "%p01", "%p02", "%p03", "%p04", "%p05",
      "%p06", "%p07", "%p08", "%p09", "%p10",
      "%p11", "%p12", "%p13", "%p14", "%p15",
      "%p16", "%p17", "%p18", "%p19", "%p20",
      "%p21", "%p22", "%p23", "%p24", "%p25",
      "%p26", "%p27", "%p28", "%p29", "%p30",
      "%p31", "%p32", "%p33", "%p34", "%p35",

      "%I00", "%I11", "%I88",
      "%G00",
      "%S00",
      "%S01",
      "%F00",

      "%O01",
      "%O02",
      "%t01", "%t02", "%U01", "%U02",

      "%U03", "%U04", "%B04",

      "%P00", "%A00", "%A01", "%SM0",
      "%S10", "%S11"};
  String id{};
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
