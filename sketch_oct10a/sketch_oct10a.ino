#include <clock.h>
#include <myWIFI.h>
#include <Arduino.h>
#include "internet.h"
#include "pin.h"
#include "globals.h"

CLOCK myclock{};
Internet inet{ &myclock };
// Настройки Wi-Fi
MyWiFi wifi{};



void processServerTask(void* param) {
  Internet* inet = (Internet*)param;
  inet->processServerResponse();  // бесконечный цикл внутри функции
  vTaskDelete(NULL);              // удаляем задачу, если вдруг выйдет
}

void setup() {
  Serial.begin(115200);
  delay(100);
  if (!SPIFFS.begin(true)) {  // затем монтируем заново
    Serial.println("❌❌❌ Ошибка монтирования SPIFFS!");
    return;
  }

  // Настройка Wi-Fi
  wifi.setup(inet.get_ssid(), inet.get_password());

  bool controlTime{ false };
  if (wifi.begin()) {
    delay(1000);
    int controlclock{};
    Serial.println("[LOG] синхронизация времени");
    while (!controlTime) {

      if (controlclock > 20) {
        Serial.println("\n❌❌❌ не смогли синхронизировать время");
        break;
      }
      delay(200);
      ++controlclock;
      controlTime = myclock.begin();
    }
  }

  inet.connect();
  xTaskCreate(
    processServerTask,  // функция задачи
    "ServerTask",       // имя задачи
    4096,               // стек
    &inet,              // параметр (указатель на объект Internet)
    1,                  // приоритет
    NULL                // handle
  );

  pinMode(22, OUTPUT);
  pinMode(23, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  pins.push_back(PIN{ 22 });
  pins.push_back(PIN{ 23 });
  pins.push_back(PIN{ 16 });
  pins.push_back(PIN{ 17 });
}

unsigned long lastSave = 0;
const unsigned long saveInterval = 60 * 1000 * 60 * 5;

void loop() {
  unsigned long now = millis();

  wifi.setup(inet.get_ssid(), inet.get_password());
  wifi.maintain();
  myclock.loop();
  for (size_t i{}; i < pins.size(); ++i) {
    bool a = pins[i].isPinActivation(myclock.getEpochMillis());
    if (now - lastSave >= saveInterval) {
      pins[i].save();
    }

    if (a) {
      digitalWrite(pins[i].get_pinNumber(), LOW);
      pins[i].set_status_pin(true);
    } else {
      digitalWrite(pins[i].get_pinNumber(), HIGH);
      pins[i].set_status_pin(false);
    }
  }
  if (now - lastSave >= saveInterval) {
    lastSave = now;
  }
  delay(2);
}
