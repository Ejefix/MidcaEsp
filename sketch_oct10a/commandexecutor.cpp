#include "commandexecutor.h"
#include "globals.h"

CommandExecutor::CommandExecutor() {}

int CommandExecutor::begin(const String &packet) {
  if (packet.length() < 4) return -2;
  String command = packet.substring(0, 4);
  int com{ -5 };
  for (int i{}; i < Skeleton::end; ++i) {
    if (command == Skeleton::commands[i]) {
      com = i;
      break;
    }
  }
  return com;
}
int CommandExecutor::playPIN(const String &packet, int pinNumber) {

  if (packet.length() < 8) return -2;
  String command = packet.substring(4, 8);

  int com{ -5 };
  for (int i{ Skeleton::accepted }; i < Skeleton::end; ++i) {
    if (command == Skeleton::commands[i]) {

      com = i;
      break;
    }
  }

  // выполняем действие
  for (size_t i{}; i < pins.size(); ++i) {
    int number = pins[i].get_pinNumber();
    if (number != pinNumber) continue;

    if (com == Skeleton::on) {
      Serial.print("[LOG] включаем pin ");
      Serial.println(number);
      pins[i].set_status_user(1);
      return 0;
    }
    if (com == Skeleton::off) {
      Serial.print("[LOG] выключаем pin ");
      Serial.println(number);
      pins[i].set_status_user(2);

      return 0;
    }
    if (com == Skeleton::script_on) {
      Serial.print("[LOG] запуск сценария для pin");
      Serial.println(number);
      pins[i].set_status_user(3);

      return 0;
    }
    if (com == Skeleton::script_off) {
      Serial.print("[LOG] выключения сценария для pin ");
      Serial.println(number);
      pins[i].set_status_user(4);

      return 0;
    }
    if (com == Skeleton::time_on) {
      Serial.print("[LOG] Добавление сценария  pin ");
      Serial.println(number);
      String first;
      first.reserve(15);
      String second;
      second.reserve(15);
      bool first_{ true };
      for (int i{ 8 }; i < packet.length(); ++i) {
        if (packet[i] == '%') {
          i += 3;
          first_ = false;
          continue;
        }
        if (first_) {
          first += packet[i];
        } else {
          second += packet[i];
        }
      }
      char *endptr;
      std::pair<unsigned long long, unsigned long long> timePair;
      timePair.first = strtoull(first.c_str(), &endptr, 10);
      if (*endptr != '\0') return -5;
      timePair.second = strtoull(second.c_str(), &endptr, 10);
      if (*endptr != '\0') return -6;
      pins[i].push_script_time(timePair);
      Serial.println("[LOG] Удачно добавили сценарий");
      return 0;
    }
    if (com == Skeleton::time_off)
    {
      String second;
      second.reserve(15);
      for (int i{ 8 }; i < packet.length(); ++i)
      {
        second += packet[i];
      }
       char *endptr;
      unsigned long long secondLong  = strtoull(second.c_str(), &endptr, 10);
       if (*endptr != '\0') return -7;
      Serial.println("[LOG] получили время которое надо удалить ->" + second);
      bool ok = pins[i].clear_script_time(secondLong);
      if(ok)
      {
        Serial.print("[LOG] Сценарий для pin ");
        Serial.print(number);
        Serial.println( " удалён.");
      }
      else
      {
        Serial.print("[LOG] Сценарий для pin ");
        Serial.print(number);
        Serial.println( " не получилось удалить.");
      }
      return 0;
    }
  }

  return -7;
}
String CommandExecutor::full_status() {
  //Serial.println("[LOG] Подготовка полного статуса");
  String body = Skeleton::commands[Skeleton::status_full];
  std::vector<String> bodyPIN;
  size_t size = pins.size();
  //Serial.print("[LOG] Количество активных PIN ");
  //Serial.println(size);
  for (size_t i{}; i < size; ++i) {
    bodyPIN.push_back(pins[i].get_status_pin());
  }
  for (auto &pinStatus : bodyPIN) {  // объединяем в одну строку
    body += pinStatus;
    body += Skeleton::commands[Skeleton::separator];
  }
  body += Skeleton::commands[Skeleton::finish];

  return body;
}
