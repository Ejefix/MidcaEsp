#include "pin.h"
#include <FS.h>
#include <SPIFFS.h>
#include "setupesp.h"
#include "globals.h"

bool PIN::changed_flags{false};
uint16_t PIN::id_pin{3001};

PIN::PIN(IPinDriver *pin_driver_) : pin_driver(pin_driver_), id{id_pin}
{
  ++id_pin;
  active = &active_power_lock;
  pending = &pending_power_lock;
}
void PIN::load()
{
}
void PIN::save() const
{
  String path = "/pin" + String(id);
  JsonDocument doc{};
  JsonArray arr = doc["pin"].to<JsonArray>();
  fill_json(arr);

  File file = SPIFFS.open(path, FILE_WRITE); // открываем файл
  if (file)
  {
    serializeJson(doc, file); // пишем в файл
    file.close();             // закрываем файл
  }
  Serial.print("[PIN] PIN сохранён:"); // лог
  Serial.println(path);
}

ExecuteResult PIN::executor(const ScheduledIntent &intent, uint8_t priority, LockPolicyType policy, timeMS endTime)
{

  if (!pin_driver)
    return ExecuteResult::DRIVER_NOT_FOUND;
  ExecuteResult rezult{};
  switch (intent.intent.type)
  {
  case ActionType::ENABLE_TOGGLE:
  case ActionType::ON:
  case ActionType::OFF:
  case ActionType::TOGGLE:
    active = &active_power_lock;
    pending = &pending_power_lock;
    break;
  case ActionType::ENABLE_FADE:
  case ActionType::FADE:
  {
    active = &active_brightness_lock;
    pending = &pending_brightness_lock;
    break;
  }
  case ActionType::CONNECT:
  {
    auto fade = std::get_if<ConnectPayload>(&intent.intent.payload);
    if (!fade)
      return ExecuteResult::INVALID_PAYLOAD;
    DeviceId device = fade->obj;
    if (device_binder->connect(fade->obj, this->id))
    {
      return ExecuteResult::SUCCESS;
    }
    return ExecuteResult::INVALID_PAYLOAD;
  }
  case ActionType::DISCONNECT:
  {
    auto fade = std::get_if<ConnectPayload>(&intent.intent.payload);
    if (!fade)
      return ExecuteResult::INVALID_PAYLOAD;
    DeviceId device = fade->obj;
    device_binder->disconnect(fade->obj, this->id);
    return ExecuteResult::SUCCESS;
  }
  case ActionType::CLEAR:
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
    activPIN = false;
    active_power_lock = {};
    pending_power_lock = {};
    active_brightness_lock = {};
    pending_brightness_lock = {};
    device_binder->disconnect(this->id);
    brightness_to = 0;
    brightness_from = 100;
    brightness = 100;
    return ExecuteResult::SUCCESS;
  }
  default:
    return ExecuteResult::UNSUPPORTED_ACTION;
  }

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

  auto promote_lock = [&](Lock *active, Lock *pending)
  {
    *pending = *active;
    *active = Lock{priority, intent.source, intent.id, policy, endTime};
  };

  switch (intent.intent.type)
  {
  case ActionType::ON:
    activPIN = true;
    promote_lock(active, pending);
    break;
  case ActionType::OFF:
    activPIN = false;
    promote_lock(active, pending);
    break;
  case ActionType::TOGGLE:
    activPIN = !activPIN;
    promote_lock(active, pending);
    break;
  case ActionType::FADE:
  {
    auto fade = std::get_if<FadePayload>(&intent.intent.payload);
    if (!fade)
      return ExecuteResult::INVALID_PAYLOAD;
    timeFADE = fade->durationMs;
    brightness_to = fade->to;
    brightness_from = fade->from;
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
    return ExecuteResult::UNSUPPORTED_ACTION;
  }
  return rezult;
}

PinId PIN::get_id() const
{
  return id;
  // TODO
}

Lock PIN::get_active_lock(ActionType type) const
{
  switch (type)
  {
  case ActionType::ON:
  case ActionType::OFF:
  case ActionType::TOGGLE:
    return active_power_lock;
  case ActionType::FADE:
    return active_brightness_lock;
  default:
    return {};
  }
}

Lock PIN::get_pending_lock(ActionType type) const
{
  switch (type)
  {
  case ActionType::ON:
  case ActionType::OFF:
  case ActionType::TOGGLE:
    return pending_power_lock;
  case ActionType::FADE:
    return pending_brightness_lock;
  default:
    return {};
  }
}

void PIN::begin()
{

  if (!pin_driver)
    return;
  pin_driver->write(activPIN, brightness);
}

void PIN::fill_json(JsonArray &arr) const
{
  JsonObject obj = arr.add<JsonObject>();
  JsonArray capsArr = obj["caps"].to<JsonArray>();
  DriverCaps caps = pin_driver->caps();
  if (caps & DriverCaps::Brightness)
    capsArr.add("brightness");

  if (caps & DriverCaps::PWM)
    capsArr.add("pwm");

  if (caps & DriverCaps::Fade)
    capsArr.add("fade");

  if (caps & DriverCaps::RGB)
    capsArr.add("rgb");

  obj["id"] = id;           // идентификатор пина
  obj["status"] = activPIN; // текущее состояние

  obj["brightness"] = brightness;
  obj["to"] = brightness_to;
  obj["from"] = brightness_from;
}

bool PIN::check_lock(const Lock &lock) const
{
  auto time = myclock.getEpochMillis();
  if (lock.policy == LockPolicyType::INFINITE || lock.endTime >= time)
    return true;
  return false;
}

Lock::Lock(uint8_t priority_, IntentSource src, ScheduledIntentID i, LockPolicyType pol, timeMS end) : priority(priority_), endTime(end), source(src), id(i), policy(pol)
{
}
