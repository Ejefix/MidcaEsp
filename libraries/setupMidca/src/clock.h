#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <time.h>

class CLOCK {
public:
    CLOCK();                        // конструктор
    bool begin();                    // запуск синхронизации
    void loop();                     // проверка и обновление времени (вызывать в loop)
    unsigned long long getEpochMillis();
    unsigned long long getEpoch_hash();
private:

    void addMilliseconds(unsigned long long ms);
    bool syncTime();                 // принудительная синхронизация с NTP

    unsigned long long lastSyncMillis{};    // когда последний раз синхронизировались
    unsigned long long lastMillis{};
    unsigned long long syncInterval = 5* 60*60 * 1000; // синхронизация каждые 5 часов
    unsigned long long timeEPS{};           // хранит последнее синхронизированное время
    
};

#endif
