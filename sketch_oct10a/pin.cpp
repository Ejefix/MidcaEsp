#include "pin.h"
#include <FS.h>
#include <SPIFFS.h>
#include "setupesp.h"

PIN::PIN(unsigned int pin) {


  if (pin == 22 || pin == 23 || pin == 16 || pin == 17) {
    pinNumber = pin;
    load();
  }
  script_time.push_back({39600000, 83000000});
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
    file.read((uint8_t *)&user_on, sizeof(user_on));
  }
  // читаем user_off
  if (file.available() >= sizeof(user_off)) {
    file.read((uint8_t *)&user_off, sizeof(user_off));
  }
  // читаем script
  if (file.available() >= sizeof(script)) {
    file.read((uint8_t *)&script, sizeof(script));
  }
  while (file.available() >= sizeof(unsigned long long) * 2) {
    unsigned long long first{}, second{};
    file.read((uint8_t *)&first, sizeof(first));
    file.read((uint8_t *)&second, sizeof(second));
    script_time.push_back({ first, second });
  }

  file.close();
}
void PIN::save() {
  Serial.println("[INF] Сохранение данных для PIN");
  String path = "/pin" + String(pinNumber);
  File file = SPIFFS.open(path, "w");  // открываем на запись (создаст новый или перезапишет)
  if (!file) return;
  file.write((const uint8_t *)&pinNumber, sizeof(pinNumber));

  // записываем bools
  file.write((const uint8_t *)&user_on, sizeof(user_on));
  file.write((const uint8_t *)&user_off, sizeof(user_off));
  file.write((const uint8_t *)&script, sizeof(script));

  // записываем пары
  for (const auto &p : script_time) {
    file.write((const uint8_t *)&p.first, sizeof(p.first));
    file.write((const uint8_t *)&p.second, sizeof(p.second));
  }
  file.close();
}

void PIN::push_script_time(std::pair<unsigned long long, unsigned long long> &time) {
  script_time.push_back(time);
}

bool PIN::clear_script_time(unsigned long long &time) {
  int size = script_time.size();
  for (int i{}; i < size; ++i) {
    if (script_time[i].second == time) {
      script_time.erase(script_time.begin() + i);  // удалить элемент
      return true;
    }
  }
  return false;
}

bool PIN::isPinActivation(const unsigned long long &time) {

  if (user_on) {
    return true;
  }
  if (user_off) {
    return false;
  }
  if (script) {
    for (auto &it : script_time) {
      if (it.second < time) {
        it.first += one_day;
        it.second += one_day;
      }
      if (it.first <= time && time <= it.second) {
        return true;
      }
    }
  }
  return false;
}

String PIN::get_status_pin() {
  String body = Skeleton::commands[pinNumber];
  if (status_pin) {
    body += Skeleton::commands[Skeleton::on];
  } else {
    body += Skeleton::commands[Skeleton::off];
  }
  if (user_on) {
    body += Skeleton::commands[Skeleton::user_on];
  }

  if (user_off) {
    body += Skeleton::commands[Skeleton::user_off];
  }
  if (script) {
    body += Skeleton::commands[Skeleton::script_on];
  } else {
    body += Skeleton::commands[Skeleton::script_off];
  }
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

void PIN::set_status_user(int user) {
  if (user == 1) {
    user_on = true;
    user_off = false;
    script = false;
  }
  if (user == 2) {
    user_off = true;
    user_on = false;
    script = false;
  }
  if (user == 3) {
    script = true;
    user_off = false;
    user_on = false;
  }
  if (user == 4) {
    script = false;
  }
}
void PIN::set_status_pin(bool status) {
  status_pin = status;
}

unsigned int PIN::get_pinNumber() {
  return pinNumber;
}
