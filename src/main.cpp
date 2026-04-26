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
  // Настройка Wi-Fi
  wifi.setup(inet.get_ssid(), inet.get_password());
  bool controlTime{false};
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
#if FW_BUILD == FW_RELAY
  I2C1.begin(GPIO_I2C1.first, GPIO_I2C1.second);
  switch1.set_pin_up(GPIO_Switch433.first);
  switch1.set_pin_duwn(GPIO_Switch433.second);
  switch1.load_from_json();
  pcf_search(I2C1);
  for (size_t i{}; i < pir_sensorG.size(); ++i)
  {
    pir_sensorG[i]->load();
  }
  for (size_t i{}; i < switch_mechanicsG.size(); ++i)
  {
    switch_mechanicsG[i].load();
  }
#endif
  for (size_t i{}; i < pinsG.size(); ++i)
  {
    pinsG[i]->load();
    pinsG[i]->default_on();
  }
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
        for (size_t i{}; i < pinsG.size(); ++i)
        {
          pinsG[i]->set_user_on(true);
        }
      }

      if (cmd == "2")
      {
        for (size_t i{}; i < pinsG.size(); ++i)
        {
          pinsG[i]->set_user_on(false);
        }
      }
      if (cmd == "3")
      {
        for (size_t i{}; i < pinsG.size(); ++i)
        {
          pinsG[i]->set_brightness(50);
        }
      }
      if (cmd == "4")
      {
        for (size_t i{}; i < pinsG.size(); ++i)
        {
          pinsG[i]->set_brightness(210);
        }
      }
      if (cmd == "5")
      {
        for (size_t i{}; i < pinsG.size(); ++i)
        {
          pinsG[i]->set_brightness(100);
        }
      }
      if (cmd == "5")
      {
        for (size_t i{}; i < pinsG.size(); ++i)
        {
          if(i % 2 == 0)
          {
            pinsG[i]->set_user_on(false);
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
#if FW_BUILD == FW_RELAY
  for (size_t i{}; i < pcfG.size(); ++i)
  {
    pcfG[i]->read();
  }
  temp_monitor.begin();
  switch1.begin();
  for (size_t i{}; i < switch_mechanicsG.size(); ++i)
  {
    switch_mechanicsG[i].begin();
  }
  for (size_t i{}; i < pir_sensorG.size(); ++i)
  {
    pir_sensorG[i]->get_activ();
  }
#endif
  wifi.setup(inet.get_ssid(), inet.get_password()); // ??????????????????????????????77
  wifi.maintain();                                  //???????????????????????????????????
  myclock.loop();

  for (size_t i{}; i < pinsG.size(); ++i)
  {
    pinsG[i]->begin();
  }

  vTaskDelay(2);
}
