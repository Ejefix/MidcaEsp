#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <time.h>
using timeMS = uint64_t;
// CLOCK — system service, must live entire runtime
class CLOCK
{
public:
   
    CLOCK() = default;
    bool begin(); // запуск синхронизации
    void loop();  // проверка и обновление времени (вызывать в loop)
    timeMS getEpochMillis();
    unsigned long long getEpoch_hash();
    operator bool() const;
    CLOCK(const CLOCK &) = delete;
    CLOCK &operator=(const CLOCK &) = delete;

    CLOCK(CLOCK &&) = delete;
    CLOCK &operator=(CLOCK &&) = delete;

private:
    void addMilliseconds(unsigned long long ms);
    bool syncTime(); // принудительная синхронизация с NTP
    bool sync{false};
    timeMS lastSyncMillis{}; // когда последний раз синхронизировались
    timeMS lastMillis{};
    timeMS syncInterval{};
    timeMS timeEPS{}; // хранит последнее синхронизированное время
};

#endif
