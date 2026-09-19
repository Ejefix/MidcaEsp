#include <Arduino.h>
#include "globals.h"

void processServerTask(void *param)
{
  Internet *inet = (Internet *)param;
  inet->processServerResponse(); // бесконечный цикл внутри функции
  vTaskDelete(NULL);             // удаляем задачу, если вдруг выйдет
}

void setup()
{
  Serial.begin(115200);
  delay(4000);
  if (!SPIFFS.begin(true))
  { // затем монтируем заново
    Serial.println("❌❌❌ Ошибка монтирования SPIFFS!");
  }
  setupStart();
  // Настройка Wi-Fi

  xTaskCreate(
      processServerTask, // функция задачи
      "ServerTask",      // имя задачи
      4096,              // стек
      &inet,             // параметр (указатель на объект Internet)
      1,                 // приоритет
      NULL               // handle
  );
  for (size_t i{}; i < pinsG.size(); ++i)
  {

    ScheduledIntent intent{};
    intent.intent.targetID = TargetRef::make(TargetType::PIN, pinsG[i]->get_id());
    intent.intent.type = ActionType::OFF;
    intent.source = IntentSource::IntentDEFAULT;
    intent.life = LifetimeType::UNENDING;
    intent.createdAt = myclock.getEpochMillis();
    store->add(intent);
  }
}
void createPinsIntents()
{
  for (size_t i{}; i < pinsG.size(); ++i)
  {
    ScheduledIntent intent{};
    intent.intent.targetID = TargetRef::make(TargetType::PIN, pinsG[i]->get_id());
    intent.intent.type = ActionType::TOGGLE;
    intent.source = IntentSource::USER;
    intent.createdAt = myclock.getEpochMillis();
    store->add(intent);
  }
}

uint32_t lastPinsIntentUpdate = 0; // время последнего запуска
void updatePinsIntentTask()
{
  uint32_t now = millis(); // текущее время

  if (now - lastPinsIntentUpdate >= 1000) // 60 секунд
  {
    createPinsIntents();        // запускаем генерацию
    lastPinsIntentUpdate = now; // обновляем таймер
  }
}
String cmd{};
size_t last_size = 0;
bool info{false};

void printInfo(uint32_t startTime)
{
  static uint32_t max_time = 0;
  static uint32_t last_print = 0;

  if (millis() - last_print > 10000)
  {
    auto current_time = millis() - startTime;
    if (current_time > max_time)
    {
      max_time = current_time;
    }
    last_print = millis();
    Serial.print("[INFO time] Поток MAIN работает max_time = ");
    Serial.println(max_time);
    Serial.print("[INFO] размер магазина ");
    Serial.print(store->size());
    Serial.println(" намериний");
    // printRAM();
  }
}
bool ram{false};
void printRAM()
{
  
  static uint32_t last_print = 0;
  if (millis() - last_print > 10000)
  {
    last_print = millis();
    
    Serial.println("\n================= RAM DUMP =================");

    // БАЗОВЫЕ МЕТРИКИ HEAP
    size_t freeHeap = ESP.getFreeHeap();
    size_t minFreeHeap = ESP.getMinFreeHeap();
    size_t heapSize = ESP.getHeapSize();

    // ФРАГМЕНТАЦИЯ
    size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    size_t freeInternal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t largestInternal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

    // PSRAM (если есть)
    size_t freePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t largestPSRAM = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

    // СТАТИСТИКА ФРАГМЕНТАЦИИ (твоя кастомная)
    static size_t minLargestBlock = SIZE_MAX;
    if (largestBlock < minLargestBlock)
      minLargestBlock = largestBlock;

    // ================= OUTPUT =================

    Serial.printf("Free heap:              %u\n", freeHeap);
    Serial.printf("Min free heap:          %u\n", minFreeHeap);
    Serial.printf("Heap size:              %u\n", heapSize);

    Serial.printf("Largest block:          %u\n", largestBlock);
    Serial.printf("Min largest block:      %u\n", minLargestBlock);

    Serial.printf("Free internal RAM:      %u\n", freeInternal);
    Serial.printf("Largest internal block: %u\n", largestInternal);

    Serial.printf("Free PSRAM:             %u\n", freePSRAM);
    Serial.printf("Largest PSRAM block:    %u\n", largestPSRAM);

    // ДИАГНОСТИКА СОСТОЯНИЯ
    Serial.println("\n--- HEALTH ---");

    if (minLargestBlock < 5000)
      Serial.println("WARNING: high fragmentation!");

    if (minFreeHeap < 50000)
      Serial.println("WARNING: low heap history!");

    if (largestBlock < 10000)
      Serial.println("WARNING: allocation risk!");

    Serial.println("===========================================\n");
  }
}
void loop()
{
  auto now = millis();
  while (Serial.available())
  {                         // есть данные
    char c = Serial.read(); // читаем символ
    if (c == '\n')
    {             // конец команды
      cmd.trim(); // убрать \r \n
      Serial.print("[RX] ");
      Serial.println(cmd); // показать что пришло

      if (cmd == "infoSTART")
      {
        info = true;
      }
      if (cmd == "infoSTOP")
      {
        info = false;
      }
      if (cmd == "ramSTART")
      {
        ram = true;
      }
      if (cmd == "ramSTOP")
      {
        ram = false;
      }
      if (cmd == "6")
      {
        Serial.print("[INFO] размер магазина ");
        Serial.print(store->size());
        Serial.println(" намериний");
      }
      cmd = ""; // очистить буфер
    }
    else
    {
      cmd += c; // собираем строку
    }
  }

  {
    device_binder->begin();
    arbitrator->begin();
    intent_executor->begin();

    for (size_t i{}; i < pinsG.size(); ++i)
    {
      pinsG[i]->begin();
    }
  }
  store->update();
  // updatePinsIntentTask();
  vTaskDelay(2);
  if (info)
    printInfo(now);
  if (ram)
    printRAM();
}
