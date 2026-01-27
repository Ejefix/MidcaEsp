#include <Arduino.h>
#include "globals.h"


void processServerTask(void* param) {
  Internet* inet = (Internet*)param;
  inet->processServerResponse();  // бесконечный цикл внутри функции
  vTaskDelete(NULL);              // удаляем задачу, если вдруг выйдет
}

const int pwmPin = 19;        // пин Gate MOSFET
const int pwmFreq = 3000;     // частота ШИМ
const int pwmResolution = 8;  // разрешение 8 бит (0-255)

#ifdef ARDUINO_ARCH_ESP32
#define CHECK "ESP32 core OK"
#else
#define CHECK "Not ESP32!"
#endif

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
  ledcAttach(pwmPin, pwmFreq, pwmResolution);
}
// Функция установки яркости
// brightness: 0 (выключено) ... 255 (максимум)
void setBrightness(int brightness) {
  if (brightness < 0) brightness = 0;
  if (brightness > 255) brightness = 255;
  ledcWrite(pwmPin, brightness);
}
int tone_{ 50 };
String cmd;
void loop() {


  while (Serial.available()) {  // есть данные
    char c = Serial.read();     // читаем символ
    if (c == '\n') {            // конец команды
      cmd.trim();               // убрать \r \n
      Serial.print("[RX] ");
      Serial.println(cmd);  // показать что пришло

      if (cmd == "1") {
        for (int i = 0; i <= 255; i++) {
          setBrightness(i);
          delay(15);
        }
        // Пример: плавное уменьшение яркости
        for (int i = 255; i >= 0; i--) {
          setBrightness(i);
          delay(15);
        }
        for (int i{}; i < 100; ++i) {
          int randBrightness = random(0, 256);  // случайное значение 0-255
          int rand = random(50, 300);  // случайное значение 0-255
          setBrightness(randBrightness);
          delay(rand);
        }
      }

      if (cmd == "2") {
        beep.startBeep(5);
      }
      if (cmd == "3") {
        tone(16, tone_);  // подаём 1000 Гц
        delay(500);       // держим полсекунды
        noTone(16);       // выключаем звук
        tone_ -= 10;
        Serial.println(tone_);
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
