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
    return;
  }
  setupStart();
  // Настройка Wi-Fi
  wifi.setup(inet.get_ssid(), inet.get_password());
  if (wifi.begin())
  {
    delay(1000);
    Serial.println("[LOG] синхронизация времени");
    if (myclock.begin())
    {
      inet.connect();
    }
    else
    {
      delay(1000);
    }
  }
  inet.startTCPSerwer();

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
uint32_t last_print = 0;

void printRAM()
{
  Serial.println("=== RAM REPORT ===");

  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());

  Serial.print("Min free heap: ");
  Serial.println(ESP.getMinFreeHeap());

  Serial.print("Heap size: ");
  Serial.println(ESP.getHeapSize());

  Serial.print("Largest block: ");
  Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

  Serial.print("Free internal RAM: ");
  Serial.println(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  Serial.print("Largest internal block: ");
  Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

  Serial.println("=================");
}
void loop()
{

  while (Serial.available())
  {                         // есть данные
    char c = Serial.read(); // читаем символ
    if (c == '\n')
    {             // конец команды
      cmd.trim(); // убрать \r \n
      Serial.print("[RX] ");
      Serial.println(cmd); // показать что пришло

      if (cmd == "1")
      {
        store->printI();
      }
      if (cmd == "2")
      {
        createPinsIntents();
      }
      if (cmd == "6")
      {

        printRAM();
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
    Serial.println(" я завис");
  }

  wifi.setup(inet.get_ssid(), inet.get_password()); // ??????????????????????????????77
  wifi.maintain();                                  //???????????????????????????????????
  myclock.loop();
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
  vTaskDelay(100);
  if (millis() - last_print > 10000)
  {
    Serial.println("Поток main работает");
    last_print = millis();
  }
}
