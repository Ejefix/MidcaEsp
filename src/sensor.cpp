#include "sensor.h"
#include <SPIFFS.h>
#include "clock.h"
#include "globals.h"

const uint64_t one_day = 24ULL * 60ULL * 60ULL * 1000ULL;
bool SENSOR::changed_flags{false};
SENSOR::SENSOR(IGpioPin *gpio_, unsigned int id_)
    : gpio(gpio_), id(id_)
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
unsigned int SENSOR::get_id() const
{
  return id;
}
void SENSOR::set_status(bool act)
{
  status = act;
  save();
   SENSOR::changed_flags = true;
}
void SENSOR::push_script_time(const std::pair<unsigned long long, unsigned long long> &time)
{
  if (time.first == 0)
  {
    script_time.first = 1;
    script_time.second = 1;
    return;
  }
  script_time = time;
  SENSOR::changed_flags = true;
}

void SENSOR::fill_json(JsonArray &arr) const
{
  JsonObject obj = arr.add<JsonObject>();
  obj["off"] = time_off_;
  obj["time"] = time_activ;
  obj["status"] = status;
  obj["reactF"] = script_time.first;
  obj["reactS"] = script_time.second;
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
        serializeJson(obj["reactF"], Serial);
        serializeJson(obj["reactS"], Serial);

        time_off_ = obj["off"] | 10000; // если ключа нет -> 10000
        time_activ = obj["time"] | 0;
        status = obj["status"] | true;
        script_time.first = obj["reactF"] | 0;
        script_time.second = obj["reactS"] | 0;
        break;
      }
    }
  }
  Serial.print("[INF] JSON загружен: для ");
  Serial.println(id);
}
void SENSOR::set_interval(const unsigned long &time_off)
{
  time_off_ = time_off;
  save();
   SENSOR::changed_flags = true;
}
bool SENSOR::get_activ()
{

  bool active = gpio->read();

  unsigned long time_now = millis();
  auto time = myclock.getEpochMillis();
  if (active)
  {
    time_on = time_now;
    time_activ = time;
    if (!status && time_now - changed_flags_time > 30000)
    {
      changed_flags_time = time_now;
       SENSOR::changed_flags = true;
    }
  }
  if (script_time.second != script_time.first)
  {
   
    while (script_time.second < time)
    {
      script_time.first += one_day;
      script_time.second += one_day;
    }
    if (script_time.first <= time && time <= script_time.second)
    {
      return false; // не реагируем в это время
    }
  }
  if (!status) // принудительно выключена реакция
  {
    return false;
  }
  if (time_now - time_on < time_off_) // ещё не прошло время отключения
  {
    return true;
  }
  return active;
}
