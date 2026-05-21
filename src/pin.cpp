#include "pin.h"
#include <FS.h>
#include <SPIFFS.h>
#include "setupesp.h"
#include "globals.h"


bool PIN::changed_flags{false};
unsigned int PIN::id_pin{3001};

PIN::PIN(IPinDriver *pin_driver_) : pin_driver(pin_driver_), id{id_pin}
{
  ++id_pin;
}
void PIN::load()
{
  Serial.print("[PIN] Загрузка данных PIN id -> ");
  Serial.println(id);
  deviceID.clear();
  String path = "/pin" + String(id);

  File file = SPIFFS.open(path, FILE_READ); // открываем файл на чтение
  if (!file)
  { // проверка открытия
    Serial.println("[ERR] файл не найден");
    return;
  }
  JsonDocument doc{};                                    // документ для JSON
  DeserializationError err = deserializeJson(doc, file); // читаем JSON
  file.close();
  if (err)
  { // проверка ошибок парсинга
    Serial.print("[ERR] JSON parse failed: ");
    Serial.println(err.c_str());
    Serial.print("file ");
    Serial.println(path);
    return;
  }
  JsonArray arr = doc["pin"].as<JsonArray>();
  if (arr.isNull() || arr.size() == 0)
    return;
  JsonObject obj = arr[0]; // первый объект в массиве
  pin_info = obj["status"];

  JsonArray deviceArr = obj["deviceID"].as<JsonArray>();
  if (deviceArr.size() == 0)
  {
    Serial.println("[PIN] Нету девайсов для добавления"); // лог
  }
  for (uint16_t id : deviceArr)
  {
    if (!add_device(id, false))
    {
      Serial.print("[PIN] Не смогли добавить девайс"); // лог
      Serial.println(id);
    }
  }
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

bool PIN::get_user_on() const
{
  return pin_info & FLAG_USER_ON;
}
bool PIN::isPinActivation(const unsigned long long &time)
{

  unsigned long time_now = millis();

  if (pin_info & FLAG_USER_ON)
  {
    return true;
  }

  if (pin_info & FLAG_USER_OFF)
  {
    return false;
  }
  if (pin_info & FLAG_SCRIPT)
  { // проверка сценариев
    
    if (pin_info & FLAG_SENSOR)
    {
      return true;
    }
  }

  //

  return false;
}
void PIN::set_status_user(unsigned char pin_info_, bool save_)
{
  pin_info &= ~(FLAG_USER_ON | FLAG_USER_OFF | FLAG_SCRIPT);
  pin_info |= (pin_info_ & (FLAG_USER_ON | FLAG_USER_OFF | FLAG_SCRIPT));
  changed_flags = true;
  if (save_)
  {
    // save();
  }
}
void PIN::set_status_pin(bool status)
{

  if (status != (pin_info & FLAG_STATUS_PIN))
  {
    Serial.print("[LOG] новый статус PIN id -> ");
    Serial.print(id);
    Serial.print(" ");
    Serial.println(status);
    changed_flags = true;
    pin_info ^= FLAG_STATUS_PIN;
  }
}
void PIN::event(const std::vector<Event> &events)
{
  bool sensor{false};
  for (auto it = events.begin(); it != events.end(); ++it)
  {
    if (it->type == DeviceType::Switch || it->type == DeviceType::Button)
    {
      handleEvent(it->event);
      continue;
    }
    if (it->type == DeviceType::Sensor &&
        (it->event == InputEvent::RisingEdge || it->event == InputEvent::HighLevel))
    {
      sensor = true;
    }
  }
  if (sensor)
  {
    if (!(pin_info & FLAG_SENSOR))
    {
      pin_info |= FLAG_SENSOR;
      changed_flags = true;
    }
  }
  else
  {
    if (pin_info & FLAG_SENSOR)
    {
      pin_info &= ~FLAG_SENSOR;
      changed_flags = true;
    }
  }
}
void PIN::handleEvent(InputEvent ev)
{
  switch (ev)
  {
  case InputEvent::NoEventErr:
    break;
  case InputEvent::NoEvent:
    break;
  case InputEvent::RisingEdge:
    break;
  case InputEvent::FallingEdge:
    break;
  case InputEvent::HighLevel:
    break;
  case InputEvent::LowLevel:
    break;
  case InputEvent::Toggle:
    pin_info ^= FLAG_USER_ON;
    break;
  case InputEvent::LongPress:
    pin_info &= ~FLAG_USER_OFF;
    pin_info &= ~FLAG_USER_ON;
    pin_info |= FLAG_SCRIPT;
    break;
  case InputEvent::DoubleClick:
    pin_info |= FLAG_USER_ON;
    break;
  }
}
unsigned int PIN::get_id() const
{
  return id;
  // TODO
}

bool PIN::add_device(DeviceId idDev, bool saved)
{
  if (!device_registry->exists(idDev))
    return false;

  device_binder->connect(idDev, this);

  deviceID.emplace(idDev);
  if (saved)
  {
    save();
    PIN::changed_flags = true;
  }
  Serial.print("[PIN] id : ");
  Serial.print(this->id);
  Serial.print(" добавил девайс id  -> ");
  Serial.println(idDev);
  return true;
}

void PIN::remove_device(DeviceId idDev)
{
  if (deviceID.find(idDev) == deviceID.end())
  {
    return;
  }
  device_binder->disconnect(idDev, this);
  deviceID.erase(idDev);
  save();
  PIN::changed_flags = true;
  Serial.print("[PIN] id : ");
  Serial.print(this->id);
  Serial.print(" удалил девайс id  -> ");
  Serial.println(idDev);
}

void PIN::set_user_on(bool status)
{
  if (status)
  {
    pin_info |= FLAG_USER_ON;
  }
  else
  {
    time_delay = millis();
    pin_info &= ~FLAG_USER_ON;
  }
  // save();
}
void PIN::begin()
{
  bool activ = isPinActivation(myclock.getEpochMillis());
  if (defaultSetup)
  {
    pin_info = 0;
    activ = false;
    if (millis() - counter_default > 15000)
    {
      defaultSetup = false;
    }
  }
  if (pin_driver)
  {
    if (pin_driver->write(activ, brightness))
      set_status_pin(activ);
  }
}
void PIN::default_on()
{
  counter_default = millis();
  defaultSetup = true;
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

  obj["id"] = id;       // идентификатор пина
  obj["status"] = pin_info; // текущее состояние

  obj["brightness"] = brightness;
  
  {
    JsonArray deviceArr = obj["deviceID"].to<JsonArray>();
    for (uint16_t id : deviceID) // перебираем set
    {
      deviceArr.add(id); // добавляем каждый ID
    }
  }
}
void PIN::set_brightness(int brightness_)
{
  if (brightness_ != this->brightness)
  {
    this->brightness = brightness_;
    PIN::changed_flags = true;
  }
}
