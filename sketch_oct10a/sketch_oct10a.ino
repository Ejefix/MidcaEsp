#include <Arduino.h>
#include "globals.h"
#include <Wire.h>  // подключаем I2C


void processServerTask(void* param) {
  Internet* inet = (Internet*)param;
  inet->processServerResponse();  // бесконечный цикл внутри функции
  vTaskDelete(NULL);              // удаляем задачу, если вдруг выйдет
}

void setup() {

  for (size_t i{}; i < pins.size(); ++i) {
    pinMode(pins[i].get_pinNumber(), OUTPUT);
  }
  for (size_t i{}; i < pin_pir_sensor.size(); ++i) {
    pinMode(pin_pir_sensor[i], INPUT);
  }
  pins[0].set_pir_sensor_interval(5000);
  pins[1].set_pir_sensor_interval(5000);

  Wire.begin(21, 22);

  /*
  Wire.begin(21, 22);
  Wire.beginTransmission(PCF1);
  Wire.write(0b11110000);
  Wire.endTransmission();
*/
  Serial.begin(115200);
  delay(20);
  if (!SPIFFS.begin(true)) {  // затем монтируем заново
    Serial.println("❌❌❌ Ошибка монтирования SPIFFS!");
    return;
  }

  // Настройка Wi-Fi
  wifi.setup(inet.get_ssid(), inet.get_password());

  bool controlTime{ false };
  if (wifi.begin()) {
    delay(500);
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
}

unsigned long lastSave = 0;
const unsigned long saveInterval = 60 * 1000 * 60 * 5;

unsigned long lastRead = 0;
const unsigned long ReadInterval = 100;
bool prev[3][3];
int save_prev{};

void loop() {
  unsigned long now = millis();

  wifi.setup(inet.get_ssid(), inet.get_password());
  wifi.maintain();
  myclock.loop();

  for (size_t i{}; i < pin_pir_sensor.size(); ++i) {
    bool pir = digitalRead(pin_pir_sensor[i]);
    pins[i].set_pir_sensor(pir);
  }

  //unsigned char pin_info_new{ 0b00000000 };
  for (size_t i{}; i < pins.size(); ++i) {
    bool a = pins[i].isPinActivation(myclock.getEpochMillis());
    if (now - lastSave >= saveInterval) {
      pins[i].save();
      lastSave = now;
    }
    if (a) {
      pins[i].set_status_pin(true);
      digitalWrite(pins[i].get_pinNumber(), LOW);
      // pin_info_new &= ~(1 << i);
    } else {

      pins[i].set_status_pin(false);
      digitalWrite(pins[i].get_pinNumber(), HIGH);
      // pin_info_new |= (1 << i);
    }
  }

  if (now - lastRead >= ReadInterval) {
    lastRead = now;
    Wire.requestFrom(PCF, (uint8_t)1);
    if (save_prev > 2) save_prev = 0;
    if (Wire.available()) {
      byte data = Wire.read();  // читаем 8 бит
      // сохраняем последние 3 состояния
      for (int i = 0; i < 3; i++) {
        prev[save_prev][i] = (bitRead(data, i) == 1);
      }
      ++save_prev;
    }
    bool stable[3];
    for (int i = 0; i < 3; i++) {
      stable[i] = (prev[0][i] == prev[1][i]) && (prev[1][i] == prev[2][i]);
    }
    // применяем к пинам только если стабильно
    if (stable[0]) {
      pins[0].set_USER_ON(prev[2][2]);
    }
    if (stable[1]) {
      pins[1].set_USER_ON(prev[2][1]);

    }
    if (stable[2]) {
      pins[3].set_USER_ON(prev[2][0]);
    }
  }  
}
