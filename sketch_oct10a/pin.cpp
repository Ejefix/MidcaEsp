#include "pin.h"
#include <FS.h>
#include <SPIFFS.h>
#include "setupesp.h"
#include "globals.h"

bool PIN::changed_flags{ false };
bool PIN::changed_script{ false };

PIN::PIN(unsigned int pin) {
  pinNumber = pin;
  load();
}
void PIN::load() {
  Serial.println("[INF] Загружаем данные для PIN");
  script_time.clear();
  std::vector<uint8_t> result;
  String path = "/pin" + String(pinNumber);

  if (!SPIFFS.exists(path)) {
    // Файл не найден
    return;
  }
  File file = SPIFFS.open(path, "r");
  if (!file) {
    // Ошибка открытия
    return;
  }
  // читаем pin
  if (file.available() >= sizeof(pinNumber)) {
    file.read((uint8_t *)&pinNumber, sizeof(pinNumber));
  }
  // читаем user_on
  if (file.available() >= sizeof(user_on)) {
    file.read((uint8_t *)&pin_type, sizeof(pin_type));
  }
  // читаем user_off
  if (file.available() >= sizeof(user_off)) {
    file.read((uint8_t *)&pin_info, sizeof(pin_info));
  }

  while (file.available() >= sizeof(unsigned long long) * 2) {
    unsigned long long first{}, second{};
    file.read((uint8_t *)&first, sizeof(first));
    file.read((uint8_t *)&second, sizeof(second));
    script_time.push_back({ first, second });
  }

  file.close();
}
void PIN::save() const {
  Serial.println("[INF] Сохранение данных для PIN");
  String path = "/pin" + String(pinNumber);
  File file = SPIFFS.open(path, "w");  // открываем на запись (создаст новый или перезапишет)
  if (!file) return;
  file.write((const uint8_t *)&pinNumber, sizeof(pinNumber));
  file.write((const uint8_t *)&pin_type, sizeof(pin_type));
  file.write((const uint8_t *)&pin_info, sizeof(pin_info));


  // записываем пары
  for (const auto &p : script_time) {
    file.write((const uint8_t *)&p.first, sizeof(p.first));
    file.write((const uint8_t *)&p.second, sizeof(p.second));
  }
  file.close();
}

void PIN::push_script_time(std::pair<unsigned long long, unsigned long long> &time) {
  script_time.push_back(time);
  PIN::changed_script = true;
  PIN::changed_flags = true;
}

bool PIN::clear_script_time(unsigned long long &time) {
  int size = script_time.size();
  for (int i{}; i < size; ++i) {
    if (script_time[i].second == time) {
      script_time.erase(script_time.begin() + i);  // удалить элемент
      PIN::changed_script = true;
      PIN::changed_flags = true;
      return true;
    }
  }
  return false;
}

bool PIN::isPinActivation(const unsigned long long &time) {


  unsigned long time_now = millis();
  if (time_now - time_delay < time_interval) {
    return false;
  }

  bool on = pin_info & FLAG_USER_ON;    // бит USER_ON
  if (on) return true;                  // если включён вручную — сразу true
  bool off = pin_info & FLAG_USER_OFF;  // бит USER_OFF
  if (off) return false;                // если выключен вручную — сразу false

  bool scr = pin_info & FLAG_SCRIPT;  // бит SCRIPT (сценарий активен)

  if (time_now - pir_sensor_time_on < time_off_)  // ещё не прошло время отключения
  {
    return true;
  }

  if (scr) {  // проверка сценариев
    for (auto &it : script_time) {
      if (it.second < time) {
        it.first += one_day;
        it.second += one_day;
        changed_script = true;
      }
      if (it.first <= time && time <= it.second) {
        return true;
      }
    }
  }
  return false;
}

String PIN::get_full_status_pin() const {
  String body = Skeleton::commands[pinNumber];
  body += (char)pin_type;  // добавляем сырой байт, символ 'A'
  body += (char)pin_info;  // добавляем байт флагов

  //Serial.print("script_time.size() ");
  //Serial.println(script_time.size());
  for (int i{}; i < script_time.size(); ++i) {
    body += Skeleton::commands[Skeleton::time_on];
    body += String((uint64_t)script_time[i].first);
    body += Skeleton::commands[Skeleton::time_off];
    body += String((uint64_t)script_time[i].second);
  }
  /*
  Serial.print("[INF] Статус PIN ");
  Serial.print(pinNumber);
  Serial.print(" ");
  Serial.println(body);*/
  return body;
}
String PIN::get_status_pin() const {
  String body = Skeleton::commands[pinNumber];
  body += (char)pin_info;
  // Serial.print("[INF] Статус PIN ");
  // Serial.println(pin_info);
  return body;
}

void PIN::set_status_user(unsigned char pin_info_) {
  pin_info &= ~(FLAG_USER_ON | FLAG_USER_OFF | FLAG_SCRIPT);
  pin_info |= (pin_info_ & (FLAG_USER_ON | FLAG_USER_OFF | FLAG_SCRIPT));

  changed_flags = true;
}
void PIN::set_status_pin(bool status) {
  bool z = pin_info & FLAG_STATUS_PIN;
  if (status != z) {
    Serial.print("[LOG] новый статус PIN ");
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
void PIN::set_pir_sensor(bool on) {
  if (on) {
    pin_info |= FLAG_PIR_SENSOR;
    pir_sensor_time_on = millis();
    time_pir_activ = myclock.getEpochMillis();
  } else {
    pin_info &= ~FLAG_PIR_SENSOR;
  }
}
void PIN::set_pir_sensor_interval(const unsigned long &time_off) {
  time_off_ = time_off;
}
void PIN::set_USER_ON(bool status) {
  bool oldState = pin_info & FLAG_SWITCH_STATE;
  bool check = oldState != status;
  if (check) {

    Serial.print("[INF] выключатель щелкнул ");
    Serial.println(pinNumber);
    if (pin_info & FLAG_USER_ON) {
      time_delay = millis();
      pin_info |= FLAG_SCRIPT;
      pin_info &= ~FLAG_USER_ON;
      pin_info &= ~FLAG_USER_OFF;

    } else if (pin_info & FLAG_USER_OFF) {
      pin_info |= FLAG_USER_ON;
      pin_info &= ~FLAG_USER_OFF;
      pin_info &= ~FLAG_SCRIPT;

    } else {
      pin_info |= FLAG_USER_ON;
      pin_info &= ~FLAG_USER_OFF;
      pin_info &= ~FLAG_SCRIPT;
    }
    if (status) {
      pin_info |= FLAG_SWITCH_STATE;
    } else {
      pin_info &= ~FLAG_SWITCH_STATE;
    }
  }
}