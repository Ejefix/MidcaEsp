#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <time.h>
using timeMS = uint64_t;
class CLOCK {
public:
    CLOCK();                        // конструктор
    bool begin();                    // запуск синхронизации
    void loop();                     // проверка и обновление времени (вызывать в loop)
    timeMS getEpochMillis();
    unsigned long long getEpoch_hash();

private:
    
    void addMilliseconds(unsigned long long ms);
    bool syncTime();                 // принудительная синхронизация с NTP

    timeMS lastSyncMillis{};    // когда последний раз синхронизировались
    timeMS lastMillis{};
    timeMS syncInterval{}; 
    timeMS timeEPS{};           // хранит последнее синхронизированное время
    
};

#endif
