#include "myWIFI.h"
#include <esp_system.h>
#include <ESPmDNS.h>

MyWiFi::MyWiFi() {
}
bool MyWiFi::begin() {
    if (!setupMyWiFi) {
        Serial.println("❌ Не удалось получить настройки  Wi-Fi");
        return false;
    }
    // Если уже подключен — выходим
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("❌❌❌ Wi-Fi уже подключен");
        return true;
    }

    // Если модуль ещё пытается подключиться — безопасно разрываем соединение
    WiFi.disconnect(true);
    delay(400);  // пауза, чтобы драйвер успел освободиться

    // Настройка режима STA
    WiFi.mode(WIFI_STA);

    // Попытка подключения
    Serial.printf("[LOG] Подключение к сети: %s\n", _ssid);
    

    WiFi.begin(_ssid, _password);

    unsigned long start = millis();
    const unsigned long timeout = 20000; // 20 секунд

    // Ждём подключения с таймаутом
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
        Serial.print(".");
        delay(500);
    }

    // Проверяем результат
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Wi-Fi подключен!");
        my_ip = WiFi.localIP().toString();
        Serial.print("[LOG] IP: "); Serial.println(WiFi.localIP());
        if(!MDNS.begin(local_MDNS))
          {
            Serial.println("mDNS error"); 
          }
        return true;
    } else {
        Serial.println("\n❌ Не удалось подключиться к Wi-Fi, перезагрузка...");
        delay(1000);
        ESP.restart();  // безопасная перезагрузка для повторной попытки
        return false;   // на практике ESP уже перезагрузится
    }
}

bool MyWiFi::maintain() {
  if (!setupMyWiFi) {  // если Wi-Fi не настроен
    Serial.println("❌Не удалось получить настройки  Wi-Fi");
    return false;      // вернуть false
  }

  if (WiFi.status() != WL_CONNECTED) {  // если соединение потеряно
    WiFi.disconnect(true);              // отключаем старое
    WiFi.begin(_ssid, _password);       // пробуем снова подключиться

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
      delay(500);  // ждём подключения до 10 сек
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\n❌ Не удалось подключиться к Wi-Fi");
      ++maxAttempts;
      if (maxAttempts > 100) {
        Serial.println("\n❌❌Не удалось подключить Wi-Fi — перезагрузка ESP");
        ESP.restart();
      }
      return false;  // не удалось подключиться
    }
    Serial.println("\n✅✅ Wi-Fi переподключен!");
    Serial.print("[LOG] IP: ");
    Serial.println(WiFi.localIP());
    my_ip = WiFi.localIP().toString();
  }
  maxAttempts = 0;
  return true;  // соединение активно
}

void MyWiFi::setup(const String &ssid, const String &password) {
  _ssid = ssid;
  _password = password;
  setupMyWiFi = true;
}

wl_status_t MyWiFi::status() {
  return WiFi.status();
}

String MyWiFi::get_my_ip() const
{
    return my_ip;
}
 