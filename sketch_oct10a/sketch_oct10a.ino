#include <Arduino.h>
#include "globals.h"


void processServerTask(void* param) {
  Internet* inet = (Internet*)param;
  inet->processServerResponse();  // бесконечный цикл внутри функции
  vTaskDelete(NULL);              // удаляем задачу, если вдруг выйдет
}


void setup() {
  delay(4000);
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {  // затем монтируем заново
    Serial.println("❌❌❌ Ошибка монтирования SPIFFS!");
    return;
  }
  // Настройка Wi-Fi
  wifi.setup(inet.get_ssid(), inet.get_password());
  bool controlTime{ false };
  if (wifi.begin()) {
    delay(1000);
    Serial.println("[LOG] синхронизация времени");
    if (myclock.begin()) {
      inet.connect();
    } else {
      delay(1000);
    }
  }
  xTaskCreate(
    processServerTask,  // функция задачи
    "ServerTask",       // имя задачи
    4096,               // стек
    &inet,              // параметр (указатель на объект Internet)
    1,                  // приоритет
    NULL                // handle
  );
  I2C1.begin(21, 22);
  switch1.set_pin(16);
  switch1.load_from_json();
  pcf_search(I2C1);
  for (size_t i{}; i < pir_sensorG.size(); ++i) {
    pir_sensorG[i]->load();
  }
  for (size_t i{}; i < pinsG.size(); ++i) {
    auto id = pinsG[i].get_pinNumber();
    if (id < 40) {
      pinMode(id, OUTPUT);
    }
    pinsG[i].load();
  }

  for (size_t i{}; i < switch_mechanicsG.size(); ++i) {
    switch_mechanicsG[i].load();
  }
 
}


String cmd;
void loop() {


  while (Serial.available()) {  // есть данные
    char c = Serial.read();     // читаем символ
    if (c == '\n') {            // конец команды
      cmd.trim();               // убрать \r \n
      Serial.print("[RX] ");
      Serial.println(cmd);  // показать что пришло

      if (cmd == "1") {
        
      }

      if (cmd == "2") {
        beep.startBeep(5);
      }
      if (cmd == "3") {
      }
      if (cmd == "err") {
        Serial.println("[OK] команда принята");
      }
      cmd = "";  // очистить буфер
    } else {
      cmd += c;  // собираем строку
    }
  }
  unsigned long now = millis();
  for (size_t i{}; i < pcfG.size(); ++i) {
    pcfG[i].read();
  }
  temp_monitor.begin();
  switch1.begin();
  wifi.setup(inet.get_ssid(), inet.get_password());  // ??????????????????????????????77
  wifi.maintain();                                   //???????????????????????????????????
  myclock.loop();
  for (size_t i{}; i < switch_mechanicsG.size(); ++i) {
    switch_mechanicsG[i].begin();
  }
  for (size_t i{}; i < pinsG.size(); ++i) {
    pinsG[i].begin();
  }
  for (size_t i{}; i < pir_sensorG.size(); ++i) {
    pir_sensorG[i]->get_activ();
  }
  beep.update();
}
