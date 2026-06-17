#include "skeleton.h"
#include <utility>
#include <FS.h>
#include <SPIFFS.h>
#include "globals.h"
const std::array<String, Skeleton::end> Skeleton::commands{
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

      "%S10",
      "%S11",
      "%S12",
      "%S13",
      "%S14",
      "%S15",
      "%S16"

    };

Skeleton::Skeleton()
{
  uint64_t esp = ESP.getEfuseMac(); // 1️⃣ Получаем уникальный MAC в 64 бит
  id = String((uint64_t)esp);
}

ExecuteResult IExecutor::execute(const ScheduledIntent &intent, uint8_t priority, LockPolicyType policy, timeMS endTime)
{
  auto promote_lock = [&](Lock *active, Lock *pending)
  {
    *pending = *active;
    *active = Lock{priority, intent.source, intent.id, policy, endTime};
  };
  ExecuteResult rezult{};
  switch (intent.intent.type)
  {
  case ActionType::ENABLE_TOGGLE:
  case ActionType::ON:
  case ActionType::OFF:
  case ActionType::TOGGLE:
  {
    active = &active_power_lock;
    pending = &pending_power_lock;
    if (check_lock(*active))
    {
      if (priority > active->priority)
      {
        rezult = ExecuteResult::SUCCESS_OVERRIDE_EQUAL_PRIORITY;
      }
      else if (priority == active->priority)
      {
        rezult = ExecuteResult::SUCCESS_OVERRIDE_LOWER_PRIORITY;
      }
      else
      {
        return ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY;
      }
    }
    else
    {
      rezult = ExecuteResult::SUCCESS;
    }
    break;
  }
  case ActionType::ENABLE_FADE:
  case ActionType::FADE:
  {
    active = &active_brightness_lock;
    pending = &pending_brightness_lock;
    if (check_lock(*active))
    {
      if (priority > active->priority)
      {
        rezult = ExecuteResult::SUCCESS_OVERRIDE_EQUAL_PRIORITY;
      }
      else if (priority == active->priority)
      {
        rezult = ExecuteResult::SUCCESS_OVERRIDE_LOWER_PRIORITY;
      }
      else
      {
        return ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY;
      }
    }
    else
    {
      rezult = ExecuteResult::SUCCESS;
    }
    break;
  }
  case ActionType::ERASE:
  { // разбита на два, что бы не смотреть чужой приоритет
    if (check_lock(active_brightness_lock))
    {
      if (active_brightness_lock.priority > priority)
      {
        return ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY;
      }
    }
    if (check_lock(active_power_lock))
    {
      if (active_power_lock.priority > priority)
      {
        return ExecuteResult::BLOCKED_BY_HIGHER_PRIORITY;
      }
    }
    break;
  }
  default:
    rezult = ExecuteResult::SUCCESS;
  }
  DeviceResult answer;
  switch (intent.intent.type)
  {
  case ActionType::ON:
  case ActionType::OFF:
  case ActionType::TOGGLE:
  case ActionType::FADE:
    answer = executeAction(intent);
    if (answer == DeviceResult::SUCCESS)
    {
      promote_lock(active, pending);
      break;
    }

  case ActionType::ENABLE_TOGGLE:
    promote_lock(active, pending);
    active_power_lock = Lock{};
    break;
  case ActionType::ENABLE_FADE:
    promote_lock(active, pending);
    active_brightness_lock = Lock{};
    break;
  default:
    answer = executeAction(intent);
  }
  switch (answer)
  {
  case DeviceResult::SUCCESS:
    if (intent.intent.type == ActionType::ERASE)
    {
      active_power_lock = {};
      pending_power_lock = {};
      active_brightness_lock = {};
      pending_brightness_lock = {};
    }
    break;
  case DeviceResult::DRIVER_NOT_FOUND:
    rezult = ExecuteResult::DRIVER_NOT_FOUND;
    break;
  case DeviceResult::INVALID_PAYLOAD:
    rezult = ExecuteResult::INVALID_PAYLOAD;
    break;
  case DeviceResult::UNSUPPORTED_ACTION:
    rezult = ExecuteResult::UNSUPPORTED_ACTION;
    break;
  default:
    break;
  }
  return rezult;
}

Lock IExecutor::get_active_lock(ActionType type) const
{
  switch (type)
  {
  case ActionType::ENABLE_TOGGLE:
  case ActionType::ON:
  case ActionType::OFF:
  case ActionType::TOGGLE:
    return active_power_lock;
  case ActionType::ENABLE_FADE:
  case ActionType::FADE:
    return active_brightness_lock;
  default:
    return {};
  }
}

Lock IExecutor::get_pending_lock(ActionType type) const
{
  switch (type)
  {
  case ActionType::ENABLE_TOGGLE:
  case ActionType::ON:
  case ActionType::OFF:
  case ActionType::TOGGLE:
    return pending_power_lock;
  case ActionType::ENABLE_FADE:
  case ActionType::FADE:
    return pending_brightness_lock;
  default:
    return {};
  }
}

bool IExecutor::check_lock(const Lock &lock) const
{
  auto time = myclock.getEpochMillis();
  if (lock.policy == LockPolicyType::INFINITE || lock.endTime >= time)
    return true;
  return false;
}

Lock::Lock(uint8_t priority_, IntentSource src, ScheduledIntentID i, LockPolicyType pol, timeMS end)
    : priority(priority_), endTime(end), source(src), id(i), policy(pol)
{
}
