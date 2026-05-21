#include <Arduino.h>
#include "globals.h"

void processServerTask(void *param)
{
  Internet *inet = (Internet *)param;
  inet->processServerResponse(); // бесконечный цикл внутри функции
  vTaskDelete(NULL);             // удаляем задачу, если вдруг выйдет
}

void setup()
{
  Serial.begin(115200);
  delay(4000);
  if (!SPIFFS.begin(true))
  { // затем монтируем заново
    Serial.println("❌❌❌ Ошибка монтирования SPIFFS!");
    return;
  }
  setupStart();
  // Настройка Wi-Fi
  wifi.setup(inet.get_ssid(), inet.get_password());
  if (wifi.begin())
  {
    delay(1000);
    Serial.println("[LOG] синхронизация времени");
    if (myclock.begin())
    {
      inet.connect();
    }
    else
    {
      delay(1000);
    }
  }
  inet.startTCPSerwer();

  xTaskCreate(
      processServerTask, // функция задачи
      "ServerTask",      // имя задачи
      4096,              // стек
      &inet,             // параметр (указатель на объект Internet)
      1,                 // приоритет
      NULL               // handle
  );
}

String cmd;

void loop()
{

  while (Serial.available())
  {                         // есть данные
    char c = Serial.read(); // читаем символ
    if (c == '\n')
    {             // конец команды
      cmd.trim(); // убрать \r \n
      Serial.print("[RX] ");
      Serial.println(cmd); // показать что пришло

      if (cmd == "1")
      {
        for (int i{}; i < 100; ++i)
        {
          device_registry->set_type(DeviceType::Sensor, i);
        }
      }

      if (cmd == "2")
      {
        for (int i{}; i < 100; ++i)
        {
          device_registry->set_type(DeviceType::Switch, i);
        }
      }
      if (cmd == "3")
      {
        for (int i{}; i < 100; ++i)
        {
          device_registry->set_type(DeviceType::Button, i);
        }
      }
      if (cmd == "4")
      {
        for (size_t i{}; i < pinsG.size(); ++i)
        {
          for (int z{}; z < 100; ++z)
          {
            pinsG[i]->remove_device(z);
          }
        }
      }
      if (cmd == "5")
      {
        for (size_t i{}; i < pinsG.size(); ++i)
        {
          for (int z{}; z < 100; ++z)
          {
            pinsG[i]->add_device(z, false);
          }
        }
      }

      if (cmd == "err")
      {
        Serial.println("[OK] команда принята");
      }

      cmd = ""; // очистить буфер
    }
    else
    {
      cmd += c; // собираем строку
    }
  }
  unsigned long now = millis();

  wifi.setup(inet.get_ssid(), inet.get_password()); // ??????????????????????????????77
  wifi.maintain();                                  //???????????????????????????????????
  myclock.loop();
  {
    
    device_binder->begin();
    for (size_t i{}; i < pinsG.size(); ++i)
    {
      pinsG[i]->begin();
    }
  }
  if(PIN::changed_flags || SENSOR::changed_flags || DeviceRegistry::changed_flags)
  {
    PIN::changed_flags = false;
    SENSOR::changed_flags = false;
    DeviceRegistry::changed_flags = false;
    updateDATA = true;
  }
  vTaskDelay(2);
}
