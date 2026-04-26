#include "commandexecutor.h"
#include "globals.h"

CommandExecutor::CommandExecutor() {}

int CommandExecutor::begin(const String &packet) const
{
  if (packet.length() < 4)
    return -2;
  String command = packet.substring(0, 4);
  int com{-5};
  for (int i{}; i < Skeleton::end; ++i)
  {
    if (command == Skeleton::commands[i])
    {
      com = i;
      break;
    }
  }
  return com;
}
int CommandExecutor::playPIN(const String &packet, int pinNumber) const
{

  size_t size = packet.length();
  if (size < 5)
    return -12;

  String command{};
  unsigned char pin_info{0};

  if (size > 5)
  {
    command = packet.substring(4, 8);
  }
  else
  {
    pin_info = static_cast<unsigned char>(packet[4]);
  }
  int com{-5};
  for (int i{Skeleton::accepted}; i < Skeleton::end; ++i)
  {
    if (command == Skeleton::commands[i])
    {

      com = i;
      break;
    }
  }

  // выполняем действие
  for (size_t i{}; i < pinsG.size(); ++i)
  {
    int number = pinsG[i]->get_pinNumber();
    if (number != pinNumber)
      continue;

    if (size == 5)
    {
      if (pin_info == 0)
      {
        return -6;
      }
      pinsG[i]->set_status_user(pin_info);
      return 0;
    }

    if (com == Skeleton::time_on)
    {
      Serial.print("[LOG] Добавление сценария  pin ");
      Serial.println(number);
      String first;
      first.reserve(15);
      String second;
      second.reserve(15);
      bool first_{true};
      for (int i{8}; i < packet.length(); ++i)
      {
        if (packet[i] == '%')
        {
          i += 3;
          first_ = false;
          continue;
        }
        if (first_)
        {
          first += packet[i];
        }
        else
        {
          second += packet[i];
        }
      }
      char *endptr;
      std::pair<unsigned long long, unsigned long long> timePair;
      timePair.first = strtoull(first.c_str(), &endptr, 10);
      if (*endptr != '\0')
        return -5;
      timePair.second = strtoull(second.c_str(), &endptr, 10);
      if (*endptr != '\0')
        return -6;
      pinsG[i]->push_script_time(timePair);
      Serial.println("[LOG] Удачно добавили сценарий");
      return 0;
    }
    if (com == Skeleton::time_off)
    {
      String second;
      second.reserve(15);
      for (int i{8}; i < packet.length(); ++i)
      {
        second += packet[i];
      }
      char *endptr;
      unsigned long long secondLong = strtoull(second.c_str(), &endptr, 10);
      if (*endptr != '\0')
        return -7;
      Serial.println("[LOG] получили время которое надо удалить ->" + second);
      bool ok = pinsG[i]->clear_script_time(secondLong);
      if (ok)
      {
        Serial.print("[LOG] Сценарий для pin ");
        Serial.print(number);
        Serial.println(" удалён.");
      }
      else
      {
        Serial.print("[LOG] Сценарий для pin ");
        Serial.print(number);
        Serial.println(" не получилось удалить.");
      }
      return 0;
    }
  }
  return -7;
}
#if FW_BUILD == FW_RELAY
int CommandExecutor::playPIR(const String &packet) const
{
  size_t size = packet.length();
  if (size < 12)
    return -3;
  String id = packet.substring(4, 8);
  PIR_SENSOR *pir{nullptr};

  for (size_t i{}; i < pir_sensorG.size(); ++i)
  {
    if (String(pir_sensorG[i]->get_id()) == id)
    {
      pir = pir_sensorG[i];
      break;
    }
  }
  if (!pir)
  {
    return -4;
  }
  String comm = packet.substring(8, 12);
  if (comm == Skeleton::commands[Skeleton::on])
  {
    pir->set_status(true); // включить PIR
    return 0;
  }
  if (comm == Skeleton::commands[Skeleton::off])
  {
    pir->set_status(false); // выключить PIR
    return 0;
  }
  if (comm == Skeleton::commands[Skeleton::time_off])
  {
    String timeStr = packet.substring(12);
    // Проверка, что только цифры
    for (size_t i = 0; i < timeStr.length(); ++i)
    {
      if (!isDigit(timeStr[i]))
      {
        return -9; // ошибка формата
      }
    }
    unsigned long timeOff = strtoul(timeStr.c_str(), nullptr, 10);
    pir->set_interval(timeOff);
    return 0;
  }
  auto pinNumber = packet.substring(12);
  unsigned int id_pin = pinNumber.toInt();
  if (comm == Skeleton::commands[Skeleton::push])
  {
    for (int z{}; z < pinsG.size(); ++z)
    {
      unsigned int number = pinsG[z]->get_pinNumber();
      if (id_pin == number)
      {
        pinsG[z]->add_pir_sensor(pir);
        Serial.print("[INF] Добавили Pir ");
        Serial.println(id);
        return 0;
      }
    }
  }
  if (comm == Skeleton::commands[Skeleton::remove])
  {
    for (int z{}; z < pinsG.size(); ++z)
    {
      unsigned int number = pinsG[z]->get_pinNumber();
      if (id_pin == number)
      {
        pinsG[z]->remove_pir_sensor(pir);
        Serial.print("[INF] Удалили Pir ");
        Serial.println(id);
        return 0;
      }
    }
    
  }
  if (comm == Skeleton::commands[Skeleton::script_on])
  {
    uint64_t value{};
    uint64_t value2{};
    String times = packet.substring(12);
    int end = times.indexOf(Skeleton::commands[Skeleton::script_off]);
    if (end != -1)
    {
      value = strtoull(times.substring(0, end).c_str(), nullptr, 10);
    }
    else
    {
      pir->push_script_time({0, 0});
      Serial.println("[INF] Удалили время сценария в Pir ");
      return 0;
    }
    
    if (times.substring(end, end + 4) == Skeleton::commands[Skeleton::script_off])
    {
      int start = end + 4;
      auto second = times.substring(start);
      value2 = strtoull(second.c_str(), nullptr, 10);
      pir->push_script_time({value, value2});
      Serial.print("[INF] Добавили время сценария в Pir ");
      Serial.print(value);
      Serial.print(" | ");
      Serial.println(value2);
      return 0;
    }
  }

  Serial.println("[INF] обработка Pir не удалась");
  return -8;
}

int CommandExecutor::playSM(const String &packet) const
{

  size_t size = packet.length();
  if (size < 12)
    return -3;
  String id = packet.substring(4, 8);
  SwitchMechanics *sm{nullptr};
  for (size_t i{}; i < switch_mechanicsG.size(); ++i)
  {
    if (String(switch_mechanicsG[i].get_id()) == id)
    {
      sm = &switch_mechanicsG[i];
      break;
    }
  }
  if (!sm)
  {
    return -10;
  }
  String comm = packet.substring(8, 12);
  String id_pin = packet.substring(12);
  unsigned long idPin = strtoul(id_pin.c_str(), nullptr, 10);
  if (comm == Skeleton::commands[Skeleton::push])
  {
    Serial.print("[INF] Добавили к SM ");
    Serial.print(id);
    Serial.print(" | реле ");
    Serial.println(idPin);
    sm->push_PIN_id(idPin);
    return 0;
  }
  if (comm == Skeleton::commands[Skeleton::remove])
  {
    Serial.print("[INF] Удалили от SM ");
    Serial.print(id);
    Serial.print(" | реле ");
    Serial.println(idPin);
    sm->remove_PIN_id(idPin);
    return 0;
  }
  return -11;
}
#endif
String CommandExecutor::full_status_json() const
{
  auto mainDoc = new DynamicJsonDocument(4096);
#if FW_BUILD == FW_RELAY
  // создаём массив switch433 в главном документе
  JsonArray arr = mainDoc->createNestedArray("433");
  switch1.fill_json(arr);
#endif
  JsonArray pinsJson = mainDoc->createNestedArray("PINS");
  for (size_t i{}; i < pinsG.size(); ++i)
  {
    pinsG[i]->fill_json(pinsJson);
  }
  #if FW_BUILD == FW_RELAY
  JsonArray pirJson = mainDoc->createNestedArray("PIR");
  for (size_t i{}; i < pir_sensorG.size(); ++i)
  {
    pir_sensorG[i]->fill_json(pirJson);
  }
  JsonArray smJson = mainDoc->createNestedArray("SM");

  for (size_t i{}; i < switch_mechanicsG.size(); ++i)
  {
    switch_mechanicsG[i].fill_json(smJson);
  }
  JsonArray temp = mainDoc->createNestedArray("T");
  temp_monitor.fill_json(temp);
#endif
  String out;
  serializeJson(*mainDoc, out); // строка для передачи
  size_t size = strlen(out.c_str());
  Serial.print("[INF] Размер данных JSON: ");
  Serial.print(size);
  Serial.println(" байт");
  delete mainDoc;
  return Skeleton::commands[Skeleton::status_full] + out; // возвращаем строку
  //return  out; // возвращаем строку
}
