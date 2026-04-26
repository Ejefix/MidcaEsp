#include "clock.h"
#include <WiFi.h>

CLOCK::CLOCK()
  : lastSyncMillis(0) {
}

bool CLOCK::begin() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  lastMillis = millis();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
  return syncTime();  // сразу синхронизируем
}

void CLOCK::loop() {
  unsigned long long nowMillis = millis();
  unsigned long long delta = nowMillis - lastMillis;
  lastMillis = nowMillis;
  // Добавляем прошедшее время к currentTime
  addMilliseconds(delta);
  // Синхронизация NTP каждые syncInterval
  if (millis() - lastSyncMillis > syncInterval) {
    syncTime();
  }
}

bool CLOCK::syncTime() {
  struct tm now;
  if (!getLocalTime(&now)) {
    syncInterval = 1 * 60 * 1000;
    Serial.println("\n❌❌❌ не смогли синхронизировать время");
    return false;  // если не удалось — выход
  }

  // сохраняем время в миллисекундах от эпохи
  time_t seconds;
  time(&seconds);                                   // получаем epoch в секундах
  timeEPS = (unsigned long long)seconds * 1000ULL;  // переводим в миллисекунды

  lastSyncMillis = millis();  // фиксируем момент синхронизации

  Serial.printf("✅ Время синхронизировано: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                now.tm_year + 1900,
                now.tm_mon + 1,
                now.tm_mday,
                now.tm_hour,
                now.tm_min,
                now.tm_sec);
  struct tm nowUtc;
  time_t seconds__ = timeEPS / 1000ULL;
  gmtime_r(&seconds__, &nowUtc);
  Serial.printf("timeEPS: %llu -> %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                timeEPS,
                nowUtc.tm_year + 1900,
                nowUtc.tm_mon + 1,
                nowUtc.tm_mday,
                nowUtc.tm_hour,
                nowUtc.tm_min,
                nowUtc.tm_sec);
  syncInterval = 5 * 60 * 60 * 1000;
  return true;
}

void CLOCK::addMilliseconds(unsigned long long ms) {
  timeEPS += ms;
}
unsigned long long CLOCK::getEpochMillis() {
  loop();
  return timeEPS;  // возвращаем результат
}
unsigned long long CLOCK::getEpoch_hash() {
  loop();
  return (timeEPS / (1000 * 60 * 4)) * (1000 * 60 * 4);
}
