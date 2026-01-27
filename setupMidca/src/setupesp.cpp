#include "setupesp.h"
#include <utility>
#include <FS.h>      // базовый файловый интерфейс
#include <SPIFFS.h>  // работа с файловой системой SPIFFS

SETUP_ESP::SETUP_ESP() {
}

bool SETUP_ESP::saveToFile(const String &path, const String &data) {
  Serial.println("[LOG] Попытка сохранить файл: " + path);  // лог начала операции

  if (!SPIFFS.begin(true)) {  // проверяем SPIFFS
    Serial.println("❌ SPIFFS не инициализирован!");
    return false;
  }

  File file = SPIFFS.open(path, FILE_WRITE);  // открываем файл
  if (!file) {
    Serial.println("❌ Не удалось открыть файл для записи: " + path);
    return false;
  }

  size_t bytesWritten = file.print(data);  // запись данных
  file.flush();                            // принудительно записываем
  file.close();                            // закрываем файл

  if (bytesWritten == 0) {
    Serial.println("❌ Ошибка записи в файл: " + path);
    Serial.printf("[INFO] Размер данных: %u, записано: %u\n", data.length(), bytesWritten);
    return false;
  }

  Serial.printf("[LOG] Успешно записано %u байт в файл: %s\n", bytesWritten, path.c_str());
  return true;
}

bool SETUP_ESP::readToFile(const String &path, String &data) {
  if (!SPIFFS.exists(path)) {  // Проверяем, есть ли файл
    return false;
  }
  File file = SPIFFS.open(path, "r");  // Открываем файл на чтение
  if (!file) {
    return false;
  }
  data = file.readString();  // Читаем всё содержимое файла
  file.close();
  return true;  // Возвращаем как String
}
void SETUP_ESP::deleteFile(const String &path) {
  if (SPIFFS.exists(path)) {  // проверяем, есть ли файл
    if (SPIFFS.remove(path))  // удаляем
      Serial.println("[LOG] Файл удалён: " + path);
    else
      Serial.println("❌ Ошибка удаления файла: " + path);
  } else {
    Serial.println("[LOG] Файл не найден: " + path);
  }
}