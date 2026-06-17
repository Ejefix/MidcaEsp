#include "sensor.h"
#include <SPIFFS.h>
#include "clock.h"
#include "globals.h"

SENSOR::SENSOR(IGpioPin *gpio_, uint16_t id_)
    : IInputDevice(id_), gpio(gpio_)
{

  path += String(id);
}
InputEvent SENSOR::event()
{
  if (!gpio)
  {
    return InputEvent::NoEventErr;
  }
  bool check = get_activ();
  if (check && active)
  {
    return InputEvent::HighLevel;
  }
  if (check && !active)
  {
    active = true;
    return InputEvent::RisingEdge;
  }
  if (!check && active)
  {
    active = false;
    return InputEvent::FallingEdge;
  }
  if (!check && !active)
  {
    return InputEvent::LowLevel;
  }
  return InputEvent::NoEvent;
}
DeviceType SENSOR::type()
{
  return DeviceType::Sensor;
}
unsigned long SENSOR::get_interval() const
{
  return time_off;
}

void SENSOR::fill_json(JsonArray &arr) const
{
  JsonObject obj = arr.add<JsonObject>();
  obj["off"] = time_off;
  obj["time"] = time_activ;
  obj["status"] = status;
}

void SENSOR::save() const
{
  Serial.println("[INF] save старт");
  Serial.print("[INF] файл: ");
  Serial.println(path);
  JsonDocument doc;

  JsonArray arr = doc[arr_name].as<JsonArray>(); // получаем массив

  fill_json(arr);
  File file = SPIFFS.open(path, FILE_WRITE); // открываем файл
  if (file)
  {
    serializeJson(doc, file); // пишем в файл
    file.close();             // закрываем файл
  }
  Serial.print("[INF] JSON сохранён: "); // лог
  Serial.println(path);
}
void SENSOR::load()
{
  Serial.print("[INF] Загрузка данных для ");
  Serial.println(id);
  File file = SPIFFS.open(path, FILE_READ); // открываем файл на чтение
  if (!file)
  { // проверка открытия
    Serial.println("[ERR] файл не найден");
    return;
  }
  JsonDocument doc{};                                    // документ для JSON
  DeserializationError err = deserializeJson(doc, file); // читаем JSON
  if (err)
  { // проверка ошибок парсинга
    Serial.print("[ERR] JSON parse failed: ");
    Serial.println(err.c_str());
    Serial.print("file ");
    Serial.println(path);
    return;
  }
  JsonArray arr = doc[arr_name].as<JsonArray>();
  if (!arr.isNull())
  {
    for (JsonObject obj : arr)
    {
      if (obj["id"].as<int>() == id)
      {
        serializeJson(obj["off"], Serial);
        serializeJson(obj["time"], Serial);
        serializeJson(obj["status"], Serial);

        time_off = obj["off"] | 10000; // если ключа нет -> 10000
        time_activ = obj["time"] | 0;
        status = obj["status"] | true;
        break;
      }
    }
  }
  Serial.print("[INF] JSON загружен: для ");
  Serial.println(id);
}

DeviceResult SENSOR::executeAction(const ScheduledIntent &intent)
{
  switch (intent.intent.type)
  {
  case ActionType::ON:
    status = true;
    ++version;
    return DeviceResult::SUCCESS;
  case ActionType::OFF:
    status = false;
    ++version;
    return DeviceResult::SUCCESS;
  case ActionType::TOGGLE:
    ++version;
    status = !status;
    return DeviceResult::SUCCESS;
  case ActionType::FADE:
  {
    auto fade = std::get_if<FadePayload>(&intent.intent.payload);
    if (!fade)
      return DeviceResult::INVALID_PAYLOAD;
    ++version;
    time_off = fade->durationMs;
    return DeviceResult::SUCCESS;
  }
  case ActionType::CONNECT:
  {
    auto fade = std::get_if<ConnectPayload>(&intent.intent.payload);
    if (!fade)
      return DeviceResult::INVALID_PAYLOAD;
    PinId pinID = fade->obj;
    DeviceId device = intent.intent.targetID;
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
    PinId pinID = fade->obj;
    DeviceId device = intent.intent.targetID;
    device_binder->disconnect(device, pinID);
    return DeviceResult::SUCCESS;
  }
  case ActionType::ERASE:
  {
    DeviceId device = intent.intent.targetID;
    device_binder->disconnect(device);
    return DeviceResult::SUCCESS;
  }
  default:
    return DeviceResult::UNSUPPORTED_ACTION;
  }
}
bool SENSOR::get_activ()
{

  bool active = gpio->read();

  if (active)
  {
    // обновление раз в 2 секунд
    if (myclock.getEpochMillis() - time_activ > 2000)
    {
      time_activ = myclock.getEpochMillis();
      ++version;
    }
  }
  if (!status) // принудительно выключена реакция
  {
    return false;
  }

  return active;
}
