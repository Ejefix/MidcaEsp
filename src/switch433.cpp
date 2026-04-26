#include "switch433.h"
#include "globals.h"
#include "pin.h"

Switch433::Switch433()
{
  signals.reserve(30);
  set_pair_signal({3851618, 4714944});
  set_pair_signal({3851620, 4714800});
}

void Switch433::set_pair_signal(const std::pair<size_t, size_t> &sig)
{
  for (int i{}; i < signalsUP.size(); ++i)
  {
    if (signalsUP[i] == sig)
    {
      Serial.println("[LOG] Signal pair уже есть");
      return;
    }
  }
  signalsUP.push_back(sig);
}

void Switch433::remove_pair_signal(const std::pair<size_t, size_t> &sig)
{
  for (int i{}; i < signalsUP.size(); ++i)
  {
    if (signalsUP[i] == sig)
    {
      signalsUP.erase(signals.begin() + i);
      return;
    }
  }
}

void Switch433::set_pin_duwn(size_t pin_)
{
  pinDuwn = pin_;
  mySwitch.enableReceive(pinDuwn);

  push_PIN(3542212, pinsG[4]); // кухня
  push_PIN(3542209, pinsG[1]); // коридор центр
}
void Switch433::set_pin_up(size_t pin_)
{
  pinUp = pin_;
  mySwitch.enableTransmit(pin_);
  mySwitch.setRepeatTransmit(5);
}
int Switch433::begin()
{
  if (pinDuwn == 0)
    return -1;
  unsigned long now = millis();
  if (signals.size() == 1)
  {
    signals[0].second = now;
  }
  if (signals.empty())
  {
    signals.emplace_back(0, now);
  }

  if (mySwitch.available())
  { // если сигнал принят

    auto signal = mySwitch.getReceivedValue();

    mySwitch.resetAvailable(); // очистить флаг
    auto it_signal = actions_pin.find(signal);
    if (signal > 50)
    {
      Serial.print("[DBG] Приёмник работает, сигнал ->");
      Serial.println(signal);
    }
    bool up{false};
    for (auto it = signalsUP.begin(); it != signalsUP.end(); ++it)
    {
      if (it->second == signal) // мы отсылаем этот сигнал, а не обрабатываем
      {
        return 0;
      }
      if (it->first == signal) // мы отсылаем этот сигнал, а не обрабатываем
      {
        up = true;
        break;
      }
    }

    if (it_signal != actions_pin.end() || up)
    {
      signals.emplace_back(signal, now);
      Serial.print("[LOG] Signal ");
      Serial.print(signal);
      Serial.println(" добален");
    }
    else
    {
      if (search)
      {

        signals_new.emplace(signal);
      }
      else
      {
        signals_new.clear();
      }
    }
  }
  if (signals.size() >= 2)
  {
    auto &last = signals.back();
    auto timestamp = last.second;
    if (now - timestamp > 200)
    {
      signals.emplace_back(0, now);
    }
  }
  checkSignal();
  if (signals.size() > 30)
  {
    signals.clear();
  }
  return 0;
}
void Switch433::push_PIN(size_t signal, PIN *pin)
{
  actions_pin[signal].insert(pin);
  // save_to_json();
}
void Switch433::save_to_json() const
{
  DynamicJsonDocument doc(1024); // JSON в памяти

  for (auto &pair : actions_pin)
  {                                                            // перебор map
    JsonArray arr = doc.createNestedArray(String(pair.first)); // ключ = signal
    for (PIN *pin : pair.second)
    {                                // перебор set<PIN*>
      arr.add(pin->get_pinNumber()); // сохраняем номер пина
    }
  }

  File file = SPIFFS.open(path, FILE_WRITE); // открываем файл
  if (file)
  {
    serializeJson(doc, file); // пишем в файл
    file.close();             // закрываем файл
  }
  Serial.println("[LOG] JSON сохранён:"); // лог
  serializeJsonPretty(doc, Serial);       // выводим JSON в Serial
  Serial.println();                       // перенос строки
}
void Switch433::load_from_json()
{
  File file = SPIFFS.open(path, FILE_READ); // открыть файл
  if (!file)
  {
    Serial.println("[ERR] JSON файл не найден");
    return;
  }
  Serial.println("[DBG] Содержимое файла:");

  DynamicJsonDocument doc(1024); // буфер JSON
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if (err)
  {
    Serial.print("[ERR] JSON ошибка: ");
    Serial.println(err.c_str());
    Serial.print("file ");
    Serial.println(path);
    return;
  }

  Serial.println("[LOG] JSON загружен из файла:");
  Serial.println("-----------------------------");

  for (JsonPair kv : doc.as<JsonObject>())
  {
    size_t signal = atoi(kv.key().c_str()); // конвертация ключа

    Serial.print("[SIGNAL] ");
    Serial.println(signal);

    JsonArray arr = kv.value().as<JsonArray>();

    for (JsonVariant v : arr)
    {
      int pinNum = v.as<int>();
      Serial.print("   └─ pin: ");
      Serial.println(pinNum);
      for (size_t i{}; i < pinsG.size(); ++i)
      {
        auto pinNum_ = pinsG[i]->get_pinNumber();
        if (pinNum_ == pinNum)
        {
          actions_pin[signal].insert(pinsG[i]);
          break;
        }
      }
    }
    Serial.println();
  }

  Serial.println("[LOG] Загрузка завершена");
  Serial.println("-----------------------------");
}
void Switch433::remove_PIN(size_t signal, int pinNum)
{
  auto it_signal = actions_pin.find(signal); // ищем ключ
  if (it_signal == actions_pin.end())
  {
    Serial.print("[LOG] Signal ");
    Serial.print(signal);
    Serial.println(" не найден");
    return;
  }

  auto &pin_set = it_signal->second; // set<PIN*>
  bool found = false;

  for (auto it = pin_set.begin(); it != pin_set.end(); ++it)
  {
    if ((*it)->get_pinNumber() == pinNum)
    {
      it = pin_set.erase(it); // удаляем
      found = true;
      break;
    }
  }

  if (found)
  {
    Serial.print("[LOG] PIN ");
    Serial.print(pinNum);
    Serial.print(" удалён из signal ");
    Serial.println(signal);
  }
  else
  {
    Serial.print("[LOG] PIN ");
    Serial.print(pinNum);
    Serial.print(" не найден в signal ");
    Serial.println(signal);
  }

  save_to_json(); // сохраняем изменения
}
void Switch433::remove_signal(size_t signal)
{
  auto it_signal = actions_pin.find(signal); // ищем ключ
  if (it_signal == actions_pin.end())
  {
    Serial.print("[LOG] Signal ");
    Serial.print(signal);
    Serial.println(" не найден");
    return;
  }
  actions_pin.erase(it_signal);
  Serial.print("[LOG] Signal ");
  Serial.print(signal);
  Serial.println(" удалён");
  for (const auto &it : actions_pin)
  {
    Serial.print("[LOG] остались Signal ");
    Serial.println(it.first);
  }
  save_to_json();
}
void Switch433::checkSignal()
{
  // Проверка минимального количества сигналов и последнего нуля
  if (signals.size() < 3 || signals.back().first != 0)
    return;
  auto size = signals.size();
  std::set<size_t> sig{};
  // собираем сигналы без повторов
  for (size_t i{}; i < size; ++i)
  {
    if (signals[i].first != 0)
    {
      sig.insert(signals[i].first);
    }
  }
  for (auto real_signal : sig)
  {
    Serial.print("[LOG] Signal ");
    Serial.print(real_signal);
    Serial.println(" обрабатывется");
  }
  if (sig.size() == 1)
  {
    auto real_signal = *sig.begin(); // единственный реальный сигнал
    if (size <= 5)
    {
      processingSignal(real_signal);
    }
    if (size > 5 && size < 9)
    {
      set_all(pin_off);
    }
    if (size > 8)
    {
      processingSignal(real_signal, true);
    }
    signals.clear(); // сигнал обработан, удаляем
  }
  else
  {
    for (auto real_signal : sig)
    {
      Serial.print("[LOG] Signal ");
      Serial.print(real_signal);
      Serial.println(" обрабатывется");
      processingSignal(real_signal);
    }
    signals.clear(); // сигнал обработан, удаляем
  }

  return;
}
void Switch433::processingSignal(size_t signal, bool script)
{
  auto it_signal = actions_pin.find(signal);
  if (it_signal != actions_pin.end())
  {
    auto pin_set = it_signal->second;
    for (auto it = pin_set.begin(); it != pin_set.end(); ++it)
    {
      if (!(*it)->get_user_on())
      {
        for (auto it = pin_set.begin(); it != pin_set.end(); ++it)
        {
          (*it)->set_user_on(true);
        }
        Serial.print("[LOG] Signal ");
        Serial.print(signal);
        Serial.println(" обработан");
        return;
      }
    }
    for (auto it = pin_set.begin(); it != pin_set.end(); ++it)
    {
      (*it)->set_user_on(false);
    }
    Serial.print("[LOG] Signal ");
    Serial.print(signal);
    Serial.println(" обработан");
  }
  else
  {
    for (auto it = signalsUP.begin(); it != signalsUP.end(); ++it)
    {
      if (it->first == signal)
      {
        Serial.print("[LOG] испускаем Signal ");
        Serial.println(it->second);
        mySwitch.send(it->second, 24);
      }
    }
  }
}
void Switch433::set_all(const unsigned char status)
{

  for (auto it_map = actions_pin.begin(); it_map != actions_pin.end(); ++it_map)
  {
    auto &pin_set = it_map->second; // set<PIN*>
    for (auto it_set = pin_set.begin(); it_set != pin_set.end(); ++it_set)
    {
      (*it_set)->set_status_user(status);
    }
  }
}
void Switch433::fill_json(JsonArray &arr) const
{
  Serial.println("[433] fill_json start"); // начало формирования JSON

  for (const auto &p : actions_pin) // перебираем сигналы с пинами
  {
    Serial.print("signal: "); // выводим сигнал
    Serial.println(p.first);

    JsonObject obj = arr.createNestedObject(); // создаём объект в массиве
    obj["signal"] = p.first;                   // записываем сигнал

    JsonArray pinsArr = obj.createNestedArray("pin"); // создаём массив пинов
    for (PIN *pin : p.second)                         // перебираем пины
    {
      int num = pin->get_pinNumber(); // получаем номер пина
      Serial.print("  pin: ");        // выводим номер пина
      Serial.println(num);
      pinsArr.add(num); // добавляем в JSON
    }
  }

  if (search) // если режим поиска включён
  {
    Serial.println("[433] search signals:"); // выводим найденные сигналы

    for (const auto &signal : signals_new) // перебираем список сигналов
    {
      Serial.print("  found: "); // вывод сигнала
      Serial.println(signal);
      arr.add(signal); // добавляем в JSON
    }
  }

  Serial.println("[433] fill_json end"); // конец формирования JSON
}
void Switch433::set_search(bool activ)
{

  search = activ;
}
unsigned int SwitchMechanics::id_switch{2001};
SwitchMechanics::SwitchMechanics(PCF8574Device *pcf_, uint8_t number_bit_)
    : pcf(pcf_), number_bit(number_bit_)
{
  id = SwitchMechanics::id_switch;
  ++SwitchMechanics::id_switch;
  path = "/sw" + String(id);
}
void SwitchMechanics::begin()
{
  bool check = get_activ();
  if (check != status)
  {
    status = check;
    // save(); TODO  только премиум или автономная система.
    for (int i{}; i < pins.size(); ++i)
    {
      if (!pins[i]->get_user_on())
      {
        //
        for (int z = i; z < pins.size(); ++z)
        {
          pins[z]->set_user_on(true);
        }
        return;
      }
    }
    for (int i{}; i < pins.size(); ++i)
    {
      pins[i]->set_user_on(false);
    }
  }
}
bool SwitchMechanics::get_activ()
{
#if FW_BUILD == FW_RELAY
  if (!pcf)
  {
    return false;
  }
  auto answer = pcf->get_bit(number_bit);
  if (answer >= 0)
  {
    return !answer;
  }
#endif
  return false;
}
unsigned int SwitchMechanics::get_id() const
{
  return id;
}
void SwitchMechanics::push_PIN_id(unsigned int id, bool save_)
{
#if FW_BUILD == FW_RELAY
  if (id == 0)
  {
    return;
  }
  if (auto *pin = search_by_id(id, pinsG)) // безопасная проверка
  {
    push_PIN(pin, save_);
  }
#endif
}
void SwitchMechanics::remove_PIN_id(unsigned int id, bool save_)
{
#if FW_BUILD == FW_RELAY
  if (id == 0)
  {
    return;
  }
  if (auto *pin = search_by_id(id, pinsG)) // безопасная проверка
  {
    remove_PIN(pin, save_);
  }
#endif
}
void SwitchMechanics::push_PIN(PIN *pin, bool save_)
{
  for (auto it = pins.begin(); it != pins.end(); ++it) // перебор vector
  {
    if (*it == pin)
      return; // сравнение указателей, если уже есть — выйти
  }
  pins.push_back(pin);
  if (save_)
  {
    save();
  }
}
void SwitchMechanics::remove_PIN(PIN *pin, bool save_)
{
  for (auto it = pins.begin(); it != pins.end(); ++it)
  {
    if (*it == pin)
    {
      pins.erase(it);
      if (save_)
      {
        save();
      }
      return;
    }
  }
}
void SwitchMechanics::fill_json(JsonArray &arr) const
{

  JsonObject obj = arr.createNestedObject();
  obj["id"] = id;
  obj["b"] = status;
  JsonArray smArr = obj.createNestedArray("pins");
  for (int i{}; i < pins.size(); ++i)
  {
    smArr.add(pins[i]->get_id());
  }
}
void SwitchMechanics::load()
{
  Serial.print("[INF] Загрузка данных SW ");
  Serial.println(id);

  if (!SPIFFS.exists(path))
    return;

  File file = SPIFFS.open(path, FILE_READ);
  if (!file)
    return;

  DynamicJsonDocument doc(512);
  auto err = deserializeJson(doc, file);
  file.close();
  if (err)
    return;
  // статус
  if (doc.containsKey("b"))
  {
    status = doc["status"].as<bool>();
  }
  if (doc.containsKey("P"))
  {
    JsonArray arr = doc["P"].as<JsonArray>();
    for (JsonVariant v : arr)
    {
      if (!v.is<JsonObject>())
        continue;
      JsonObject obj = v.as<JsonObject>();
      if (obj.containsKey("pins"))
      {
        JsonArray pinsArr = obj["pins"].as<JsonArray>();
        for (JsonVariant pin_v : pinsArr)
        {
          int pin_id = pin_v.as<int>();
          push_PIN_id(pin_id, false); // безопасно добавляем пины
        }
      }
    }
  }
  Serial.println("[INF] JSON SW загружен:");
  serializeJsonPretty(doc, Serial); // выводим JSON для отладки
  Serial.println();
}
void SwitchMechanics::save() const
{
  DynamicJsonDocument doc(512);

  JsonObject obj = doc.to<JsonObject>();
  obj["b"] = status; // сохраняем статус

  JsonArray arr = obj.createNestedArray("P");
  fill_json(arr); // сохраняем id пинов

  File file = SPIFFS.open(path, FILE_WRITE);
  if (file)
  {
    serializeJson(doc, file);
    file.close();
  }
}
