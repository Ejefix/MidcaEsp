#include "commandexecutor.h"
#include "globals.h"
#include "sensor.h"

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
/*
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
    int number = pinsG[i]->get_id();

    if (number != pinNumber)
      continue;
    if (size == 5)
    {

      
      return 0;
    }
    if (com == Skeleton::brightness)
    {
      String second;
      second.reserve(3);
      for (int i{8}; i < packet.length(); ++i)
      {
        second += packet[i];
      }
      Serial.println("[LOG] ----------------------------------------получили->" + packet);
      char *endptr;
      int  secondLong = strtoull(second.c_str(), &endptr, 10);
      if (*endptr != '\0')
        return -7;
      Serial.println("[LOG] ----------------------------------------получили яркость для установки ->" + second);
      
      return 0;
    }
  }
    */
  return -7;
  
}

int CommandExecutor::playSensor(const String &packet) const
{
  /*
  size_t size = packet.length();
  if (size < 12)
    return -3;
  String id = packet.substring(4, 8);
  unsigned long idSensor = strtoul(id.c_str(), nullptr, 10);

  SENSOR *sensor{nullptr};

  if (device_registry->exists(idSensor))
  {
    auto dev = device_registry->get(idSensor);
    sensor = dynamic_cast<SENSOR *>(dev);
  }

  if (!sensor)
  {
    return -4;
  }
  String comm = packet.substring(8, 12);
  if (comm == Skeleton::commands[Skeleton::on])
  {
   
    return 0;
  }
  if (comm == Skeleton::commands[Skeleton::off])
  {
    
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
    
    return 0;
  }
  auto pinNumber = packet.substring(12);
  unsigned int id_pin = pinNumber.toInt();

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
      
      Serial.println("[INF] Удалили время сценария в sensor ");
      return 0;
    }

    if (times.substring(end, end + 4) == Skeleton::commands[Skeleton::script_off])
    {
      int start = end + 4;
      auto second = times.substring(start);
      value2 = strtoull(second.c_str(), nullptr, 10);
      
      Serial.print("[INF] Добавили время сценария в sensor ");
      Serial.print(value);
      Serial.print(" | ");
      Serial.println(value2);
      return 0;
    }
  }

  Serial.println("[INF] обработка Pir не удалась");
  */
  return -8;
}

String CommandExecutor::full_status_json() const
{/*
  JsonDocument mainDoc{};
  mainDoc["ID"] = id;
  mainDoc["time"] = millis();
  JsonObject data = mainDoc["data"].to<JsonObject>();

  JsonArray pinsJson = data["PINS"].to<JsonArray>();
  for (size_t i{}; i < pinsG.size(); ++i)
  {
    pinsG[i]->fill_json(pinsJson);
  }
  JsonArray deviceJson = data["DEVICE"].to<JsonArray>();
  device_registry->fill_json(deviceJson);
  JsonArray storeJson = data["STORE"].to<JsonArray>();
  store->fill_json(storeJson);
  
  String out;
  serializeJson(mainDoc, out); // строка для передачи
  size_t size = strlen(out.c_str());
 // Serial.print("[INF] Размер данных JSON: ");
 // Serial.print(size);
 // Serial.println(" байт");
  return Skeleton::commands[Skeleton::status_full] + out; // возвращаем строку
  // return  out; // возвращаем строку
  */
 return {};
}
