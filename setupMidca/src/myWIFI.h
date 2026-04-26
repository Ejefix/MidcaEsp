#ifndef MYWIFI_H
#define MYWIFI_H
#include <Arduino.h>
#include <WiFi.h>

class MyWiFi {
public:
    MyWiFi(); // Конструктор с данными сети
    bool begin();                                   // Подключение к Wi-Fi
    bool maintain();                                // Поддержка соединения, переподключение
    wl_status_t status();                           // Статус соединения
    void setup(const String &ssid, const String &password);
private:
    String _ssid;
    String _password;
    bool setupMyWiFi{false};
    unsigned long _lastAttempt;
    int maxAttempts{ 0 }; 
};

#endif
