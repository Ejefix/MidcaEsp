#include "pin.h"
#include <FS.h>
#include <SPIFFS.h>
#include "setupesp.h"
#include "globals.h"


uint16_t PIN::id_pin{3001};

PIN::PIN(IPinDriver *pin_driver_) : pin_driver(pin_driver_), id{id_pin}
{
  ++id_pin;
  if (timeFADE == 0)
  {
    brightness = brightness_to;
  }
  else
  {
    step = (brightness_to - brightness_from) / static_cast<double>(timeFADE);
  }
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

uint32_t PIN::get_version() const
{
  return version;
}

DeviceResult PIN::executeAction(const ScheduledIntent &intent)
{

  if (!pin_driver)
    return DeviceResult::DRIVER_NOT_FOUND;
  switch (intent.intent.type)
  {
  case ActionType::ON:
    activPIN = true;
    return DeviceResult::SUCCESS;
  case ActionType::OFF:
    activPIN = false;
    return DeviceResult::SUCCESS;
  case ActionType::TOGGLE:
    activPIN = !activPIN;
    return DeviceResult::SUCCESS;
  case ActionType::FADE:
  {
    auto fade = std::get_if<FadePayload>(&intent.intent.payload);
    if (!fade)
      return DeviceResult::INVALID_PAYLOAD;
    timeFADE = fade->durationMs;
    brightness_to = fade->to;
    brightness_from = fade->from;
    if(brightness_from > brightness_to)
    {
      std::swap(brightness_from, brightness_to);
    }
    if (timeFADE == 0)
    {
      brightness = brightness_to;
    }
    else
    {
      step = (brightness_to - brightness_from) / static_cast<double>(timeFADE);
    }
    return DeviceResult::SUCCESS;
  }
  case ActionType::CONNECT:
  {
    auto fade = std::get_if<ConnectPayload>(&intent.intent.payload);
    if (!fade)
      return DeviceResult::INVALID_PAYLOAD;
    DeviceId device = fade->obj;
    PinId pinID = this->id;
    if (device_binder->connect(device, pinID))
    {
      return DeviceResult::SUCCESS;
    }
    return DeviceResult::INVALID_PAYLOAD;
  }
  case ActionType::DISCONNECT:
  {
    auto fade = std::get_if<ConnectPayload>(&intent.intent.payload);
    if (!fade)
      return DeviceResult::INVALID_PAYLOAD;
    DeviceId device = fade->obj;
    PinId pinID = this->id;
    device_binder->disconnect(device, pinID);
    return DeviceResult::SUCCESS;
  }
  case ActionType::ERASE:
  {
    PinId pinID = this->id;
    device_binder->disconnect(pinID);
    return DeviceResult::SUCCESS;
  }
  default:
    return DeviceResult::UNSUPPORTED_ACTION;
  }
}

PinId PIN::get_id() const
{
  return id;
  // TODO
}

void PIN::begin()
{

  if (!pin_driver)
    return;
  if (!activPIN)
  {
    brightness = brightness_from;
  }
  else
  {
    if (timeFADE == 0)
    {
      brightness = brightness_to;
    }
    else
    {
      auto now = millis();
      auto elapsed = now - timeStep;
      timeStep = now;

      brightness += step * elapsed;

      if ((step > 0 && brightness > brightness_to) || (step < 0 && brightness < brightness_to))
      {
        brightness = brightness_to;
      }
    }
  }

  // если драйвер принял изменения, увеличиваем версию для синхронизации
  if (pin_driver->write(activPIN, static_cast<uint8_t>(brightness)))
  {
    ++version;
  }
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
