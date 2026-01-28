#include "pin.h"
#include <FS.h>
#include <SPIFFS.h>
#include "setupesp.h"
#include "globals.h"

const uint64_t one_day = 24ULL * 60ULL * 60ULL * 1000ULL;
bool PIN::changed_flags{ false };
unsigned int PIR_SENSOR::id_s{ 1101 };
unsigned int PIN::id_pin{ 3001 };
bool FlowMeter::on_off{ false };
bool PIR_SENSOR::changed_flags{ false };

PIR_SENSOR::PIR_SENSOR(unsigned int pinNumber_)
  : pcf(nullptr), pinNumber(pinNumber_), number_bit(0), id(PIR_SENSOR::id_s) {

  ++PIR_SENSOR::id_s;
  path = "/pir" + String(id);
  pinMode(pinNumber_, INPUT);
}
PIR_SENSOR::PIR_SENSOR(PCF8574Device &pcf_, uint8_t number_bit_)
  : pcf(&pcf_), pinNumber(0), number_bit(number_bit_), id(PIR_SENSOR::id_s) {
  ++PIR_SENSOR::id_s;
  path = "/pir" + String(id);
}
unsigned int PIR_SENSOR::get_id() const {
  return id;
}
void PIR_SENSOR::set_status(bool act) {
  status = act;
  PIN::changed_flags = true;
  save();
}

void PIR_SENSOR::push_script_time(std::pair<unsigned long long, unsigned long long> &time) {
  script_time = time;
}

void PIR_SENSOR::fill_json(JsonArray &arr) const {

  JsonObject obj = arr.createNestedObject();
  obj["off"] = time_off_;
  obj["id"] = id;
  obj["time"] = time_activ;
  obj["status"] = status;
}
MQ_SENSOR::MQ_SENSOR(PCF8574Device &pcf_, uint8_t number_bit_, BEEP &beep_)
  : beep(&beep_), PIR_SENSOR(pcf_, number_bit_) {
  arr_name = "mq";
  set_interval(5000);
}
bool MQ_SENSOR::get_activ() {
  bool active = PIR_SENSOR::get_activ();
  if (active && beep && !beep->get_activ()) {
    beep->gasBeep();
  }
  return active;
}
void MQ_SENSOR::set_status(bool act) {
  return;
}
PIN::PIN(unsigned int pin, RelayFunc func)
  : pinNumber(pin), startRelay(func),
    pcf(nullptr), number_bit(0),
    id(pin) {
}
PIN::PIN(PCF8574Device &pcf_, uint8_t number_bit_)
  : pinNumber(PIN::id_pin), startRelay(nullptr),
    pcf(&pcf_), number_bit(number_bit_),
    id(PIN::id_pin) {
  ++PIN::id_pin;
}
void PIR_SENSOR::save() const {
  Serial.println("[INF] save старт");
  Serial.print("[INF] файл: ");
  Serial.println(path);
  DynamicJsonDocument doc(512);  // JSON в памяти
  JsonArray arr = doc.createNestedArray(arr_name);
  fill_json(arr);
  File file = SPIFFS.open(path, FILE_WRITE);  // открываем файл
  if (file) {
    serializeJson(doc, file);  // пишем в файл
    file.close();              // закрываем файл
  }
  Serial.print("[INF] JSON сохранён: ");  // лог
  Serial.println(path);
}
void PIR_SENSOR::load() {
  Serial.print("[INF] Загрузка данных для ");
  Serial.println(id);
  File file = SPIFFS.open(path, FILE_READ);  // открываем файл на чтение
  if (!file) {                               // проверка открытия
    Serial.println("[ERR] файл не найден");
    return;
  }
  DynamicJsonDocument doc(512);                           // документ для JSON
  DeserializationError err = deserializeJson(doc, file);  // читаем JSON
  if (err) {                                              // проверка ошибок парсинга
    Serial.print("[ERR] JSON parse failed: ");
    Serial.println(err.c_str());
    Serial.print("file ");
    Serial.println(path);
    return;
  }
  JsonArray arr = doc[arr_name].as<JsonArray>();
  if (!arr.isNull()) {
    for (JsonObject obj : arr) {
      if (obj["id"].as<int>() == id) {
        serializeJson(obj["off"], Serial);
        serializeJson(obj["time"], Serial);
        serializeJson(obj["status"], Serial);
        time_off_ = obj.containsKey("off")
                      ? obj["off"].as<unsigned long>()
                      : 10000;
        time_activ = obj.containsKey("time")
                       ? obj["time"].as<uint64_t>()
                       : 0;
        status = obj.containsKey("status")
                   ? obj["status"].as<bool>()
                   : true;
        break;
      }
    }
  }
  Serial.print("[INF] JSON загружен: для ");
  Serial.println(id);
}
void PIN::load() {
  Serial.print("[INF] Загрузка данных PIN ");
  Serial.println(pinNumber);
  script_time.clear();  // очищаем сценарий
  pirs.clear();
  String path = "/pin" + String(pinNumber);

  File file = SPIFFS.open(path, FILE_READ);  // открываем файл на чтение
  if (!file) {                               // проверка открытия
    Serial.println("[ERR] файл не найден");
    return;
  }
  DynamicJsonDocument doc(2048);                          // документ для JSON
  DeserializationError err = deserializeJson(doc, file);  // читаем JSON
  file.close();
  if (err) {  // проверка ошибок парсинга
    Serial.print("[ERR] JSON parse failed: ");
    Serial.println(err.c_str());
    Serial.print("file ");
    Serial.println(path);
    return;
  }
  JsonArray arr = doc["pin"].as<JsonArray>();
  if (!arr.isNull() && arr.size() > 0) {
    JsonObject obj = arr[0];  // первый объект в массиве
    pin_info = obj["status"];

    for (JsonObject t : obj["script"].as<JsonArray>()) {
      script_time.emplace_back(t["on"].as<unsigned long long>(), t["off"].as<unsigned long long>());
    }
    for (int id : obj["pir"].as<JsonArray>()) {
      add__pir_sensor_id(id, false);
    }
  }
}
void PIN::save() const {

  Serial.println("[INF] PIN save старт");

  String path = "/pin" + String(pinNumber);
  Serial.print("[INF] файл: ");
  Serial.println(path);
  DynamicJsonDocument doc(2048);  // JSON в памяти
  JsonArray arr = doc.createNestedArray("pin");
  fill_json(arr);

  File file = SPIFFS.open(path, FILE_WRITE);  // открываем файл
  if (file) {
    serializeJson(doc, file);  // пишем в файл
    file.close();              // закрываем файл
  }
  Serial.println("[INF] PIN сохранён:");  // лог
  Serial.println(path);
}
void PIN::push_script_time(std::pair<unsigned long long, unsigned long long> &time) {
  script_time.push_back(time);
  PIN::changed_flags = true;
  save();
}
bool PIN::clear_script_time(unsigned long long &time) {
  int size = script_time.size();
  for (int i{}; i < size; ++i) {
    if (script_time[i].second == time) {
      script_time.erase(script_time.begin() + i);  // удалить элемент
      PIN::changed_flags = true;
      save();
      return true;
    }
  }
  return false;
}
bool PIN::get_user_on() const {
  return pin_info & FLAG_USER_ON;
}
bool PIN::isPinActivation(const unsigned long long &time) {

  unsigned long time_now = millis();
  if (time_now - time_delay < time_interval) {
    return false;  // после выключения механического выключателя
  }
  if (pin_info & FLAG_USER_ON) {

    return true;
  }

  if (pin_info & FLAG_USER_OFF) {
    return false;
  }
  if (pin_info & FLAG_SCRIPT) {  // проверка сценариев
    for (auto &it : script_time) {
      if (it.second < time) {
        it.first += one_day;
        it.second += one_day;
      }
      if (it.first <= time && time <= it.second) {
        return true;
      }
    }

    // если добавляем сценарий выключения, то на датчик не реагируем
    if (pin_info & FLAG_PIR_SENSOR) {
      return true;
    }
  }
  return false;
}
void PIN::set_status_user(unsigned char pin_info_, bool save_) {
  pin_info &= ~(FLAG_USER_ON | FLAG_USER_OFF | FLAG_SCRIPT);
  pin_info |= (pin_info_ & (FLAG_USER_ON | FLAG_USER_OFF | FLAG_SCRIPT));
  changed_flags = true;
  if (save_) {
    save();
  }
}
void PIN::set_status_pin(bool status) {
  bool z = pin_info & FLAG_STATUS_PIN;
  if (status != z) {
    Serial.print("[LOG] новый статус PIN ");
    Serial.print(pinNumber);
    Serial.print(" ");
    Serial.println(status);
    changed_flags = true;
    if (status) {
      pin_info |= FLAG_STATUS_PIN;
    } else {
      pin_info &= ~FLAG_STATUS_PIN;
    }
  }
}
unsigned int PIN::get_pinNumber() const {
  return pinNumber;
}
unsigned int PIN::get_id() const {
  return get_pinNumber();
  //TODO
}
void PIN::set_pir_sensor() {

  for (size_t i{}; i < pirs.size(); ++i) {
    if (pirs[i]->get_activ()) {
      pin_info |= FLAG_PIR_SENSOR;

      return;
    }
  }
  pin_info &= ~FLAG_PIR_SENSOR;
}
void PIN::set_user_on(bool status) {
  if (status) {
    pin_info |= FLAG_USER_ON;
  } else {
    time_delay = millis();
    pin_info &= ~FLAG_USER_ON;
  }
  save();
}
void PIN::begin() {

  set_pir_sensor();
  bool activ = isPinActivation(myclock.getEpochMillis());
  if (startRelay) {
    bool status = !digitalRead(pinNumber);
    if (status != activ) {
      startRelay(pinNumber, activ);
    }
  }
  set_status_pin(activ);
}
void PIR_SENSOR::set_interval(const unsigned long &time_off) {
  time_off_ = time_off;
  save();
}
bool PIR_SENSOR::get_activ() {
  if (pinNumber == 0 && !pcf) {
    return false;
  }
  bool active;

  if (!pcf) {
    active = digitalRead(pinNumber);
  } else {
    auto bit_ = pcf->get_bit(number_bit);
    if (bit_ == 0) {
      active = true;
    } else {
      active = false;
    }
  }
  unsigned long time_now = millis();
  if (active) {
    time_on = time_now;
    time_activ = myclock.getEpochMillis();
    if (!status && time_now - changed_flags_time > 60000) {
      PIR_SENSOR::changed_flags = true;
      changed_flags_time = time_now;
    }
  }
  auto time = myclock.getEpochMillis();
  
  if (!status)  // принудительно выключена реакция
  {
    return false;
  }
  if (time_now - time_on < time_off_)  // ещё не прошло время отключения
  {
    return true;
  }
  return active;
}
void PIN::fill_json(JsonArray &arr) const {
  JsonObject obj = arr.createNestedObject();

  obj["number"] = id;        // идентификатор пина
  obj["status"] = pin_info;  // текущее состояние
  {
    // массив времени сценариев
    JsonArray timesArr = obj.createNestedArray("script");
    for (const auto &p : script_time) {
      JsonObject t = timesArr.createNestedObject();
      t["on"] = p.first;
      t["off"] = p.second;
    }
  }
  {
    JsonArray pirArr = obj.createNestedArray("pir");
    for (const auto &pir : pirs) {
      pirArr.add(pir->get_id());  // добавляем ID каждого
    }
  }
}
void PIN::add__pir_sensor_id(int id, bool save_) {

  if (auto *pir = search_by_id(id, pir_sensorG))  // безопасная проверка
  {
    add_pir_sensor(pir, save_);
  }
}
void PIN::remove__pir_sensor_id(int id, bool save_) {
  if (auto *pir = search_by_id(id, pir_sensorG))  // безопасная проверка
  {
    remove_pir_sensor(pir, save_);
  }
}
void PIN::add_pir_sensor(PIR_SENSOR *pir, bool save_) {
  for (auto it = pirs.begin(); it != pirs.end(); ++it)  // перебор vector
  {
    if (*it == pir) return;  // сравнение указателей, если уже есть — выйти
  }
  pirs.push_back(pir);  // добавить новый PIR_SENSOR*
  if (save_) {
    save();
  }
}
void PIN::remove_pir_sensor(PIR_SENSOR *pir, bool save_) {
  for (auto it = pirs.begin(); it != pirs.end(); ++it) {
    if (*it == pir) {
      pirs.erase(it);
      if (save_) {
        save();
      }
      return;
    }
  }
}
BEEP::BEEP(unsigned int pin, RelayFunc func, unsigned int frequency_)
  : pinNumber(pin),  // Сохраняем номер пина
    startRelay(func), frequency(frequency_) {
  pinMode(pin, OUTPUT);
}
void BEEP::startBeep(unsigned long count) {
  if (stop) return;
  interval = 120;
  start(count);
}
void BEEP::gasBeep() {
  interval = 700;
  start(5);
}
void BEEP::start(int times) {
  if (pinNumber == 0) return;
  active = true;
  step = 0;
  totalSteps = times * 2;  // каждый сигнал = включение + пауза
  state = true;
  time_beep_start = millis();
}
void BEEP::update() {
  unsigned long now = millis();

  if (active && now - time_beep_start >= interval) {
    if (frequency == 0) {
      startRelay(pinNumber, !state);  // переключаем пищалку , обычное вкл выкл
    } else {
      if (state) {
        tone(pinNumber, frequency);
      } else {
        noTone(pinNumber);
      }
    }
    state = !state;
    time_beep_start = now;
    ++step;
    if (step >= totalSteps) active = false;  // сигналы закончились
  }
}
bool BEEP::get_activ() {
  return active;
}
void BEEP::set_stop(bool stop_) {
  stop = stop_;
}
FlowMeter *FlowMeter::instance = nullptr;
FlowMeter::FlowMeter(uint8_t pin, PIN *pin_)
  : _pin(pin), pinWater(pin_) {
}
void FlowMeter::setup() {
  pinMode(_pin, INPUT_PULLUP);                     // вход с подтяжкой
  instance = this;                                 // сохраняем объект
  attachInterrupt(_pin, FlowMeter::isr, FALLING);  // прерывание по импульсу
}
void IRAM_ATTR FlowMeter::isr() {
  ++instance->pulses;
}
void FlowMeter::begin() {

  if (on_off) {
    return;
  }
  // Фиксируем начало утечки, если ещё не зафиксирована и есть импульсы
  if (!leak && pulses > 0 && startTime == 0) {
    Serial.println("[INF] утечка зафиксирована");
    startTime = millis();  // сохраняем время начала учёта
    pulses = 0;            // сброс счётчика импульсов
    leak = true;           // ставим флаг утечки
  }

  if (leak && (millis() - startTime) >= interval) {
    startTime = millis();
    if (pulses > 0) {  // импульсы есть → утечка продолжается
      ++leakCount;
      Serial.print("[INF] утечка зафиксирована ");
      Serial.println(leakCount);
      noLeakCount = 0;  // сброс интервалов без утечки
    } else {            // импульсов нет
      ++noLeakCount;
    }
    pulses = 0;
    if (noLeakCount >= 3) {
      if (close && openWater()) {
        leak = false;
        leakCount = 0;
        noLeakCount = 0;
        startTime = 0;
      }
    }

    if (leakCount == 6) {
      // beep.startBeep(1);
    }
    if (leakCount == 8) {
      //beep.startBeep(2);
    }
    if (leakCount >= 10) {
      if (open && closeWater()) {
        // beep.startBeep(3);
        open = false;
      }
    }
  }
}
bool FlowMeter::openWater() {
  if (millis() - closeTime > 12000) {
    Serial.println("[INF] открытие воды");
    unsigned char pin_info{ 0b10000100 };
    pinWater->set_status_user(pin_info, false);
    openTime = millis();
    open = true;
    return true;
  }
  return false;
}
bool FlowMeter::closeWater() {
  if (millis() - openTime > 12000) {
    Serial.println("[INF] перекрытие воды");
    unsigned char pin_info{ 0b10000010 };
    pinWater->set_status_user(pin_info, false);
    closeTime = millis();
    close = true;
    return true;
  }
  return false;
}
