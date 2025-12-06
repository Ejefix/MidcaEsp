#include <HTTPClient.h>
#include <Arduino.h>
#include "internet.h"
#include <WiFiClientSecure.h>
#include "time.h"
#include <memory>
#include "setupesp.h"
#include <utility>
#include "commandexecutor.h"
#include "pin.h"
bool serverOK = true;

const unsigned long long Internet::epochDiff{ 1000 * 60 * 4 };


Internet::Internet(CLOCK* myclock)
  : myclock(myclock) {
}


// это нужно для первого подлючения к серверу и авторизации
bool Internet::connect() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌❌ Wi-Fi не подключен!");
    return false;
  }
  client.stop();
  if (client.connect("176.60.208.46", 64525)) {
    Serial.println("✅ Соединение с сервером установлено");
    client.print(body());
    client.flush();
    unsigned long start = millis();
    const unsigned long timeout = 5000;  // максимум 5 сек ожидания
    while (true) {
      if (client.available()) {
        int command = get_command();
        if (command < 0) {
          delay(400);
          yield();
          while (client.available() > 0) {
            client.read();  // читаем и отбрасываем
          }
        }
        if (command >= 0) {
          if (command == Skeleton::accepted) {
            Serial.println("✅ Авторизация прошла успешно");
            serverOK = true;
            return true;
          } else {
            Serial.println("❌ Ошибка авторизации");
            return false;
          }
        }
        if (millis() - start > timeout) {
          Serial.println("⚠️ Таймаут ожидания ответа от сервера");
          break;
        }
      }
      delay(5);
    }
    return false;
  } else {
    Serial.println("❌ Не удалось подключиться к серверу");
    return false;
  }
}
String Internet::body() {

  Serial.println("[LOG] Отправка авторизации серверу");
  String body = Skeleton::commands[Skeleton::idESP];
  int sizeID = Skeleton::id.length();
  body += String((int)sizeID);
  Serial.print("[LOG] 🔗 idEsp-> ");
  Serial.println(Skeleton::id);
  body += Skeleton::id;
  String secret = "?6G:N0$ua;0n[?hu";
  unsigned long long epoch = myclock->getEpoch_hash();

  String hash = Encryption::get_hash(epoch, secret, Skeleton::id);
  body += hash;
  return body;
}


int Internet::get_command() {

  size_t size{};
  char* data = read_buffer(size);
  String packet(data, size);
  delete[] data;
  if (packet == Skeleton::commands[Skeleton::accepted]) {
    return Skeleton::accepted;
  } else {
    return -24;
  }
}

int countProcess{ 0 };
void Internet::processServerResponse() {

  CommandExecutor comEx;
  while (true) {
    ++countProcess;
    delay(5);
    if (client.available()) {
      size_t size{};
      char* data = read_buffer(size);
      if (data == nullptr) {
        continue;
      }
      String packet(data, size);
      String decrypted = Encryption::decrypt(packet);  // <-- передать сюда data
      delete[] data;
      Serial.println("[LOG] получена команда -> " + decrypted);
      int comm = comEx.begin(decrypted);
      if (comm == Skeleton::idSer) {
        Serial.println("[INF] Ping <-> Pong");
        serverOK = true;
      }
      if (comm == Skeleton::status_full) {
        String body = Encryption::encrypt(comEx.full_status_pins());
        client.print(body);
        client.flush();
      }
      if (comm >= Skeleton::pin1 && comm <= Skeleton::pin35) {
        int answer = comEx.playPIN(decrypted, comm);
        delay(1);
        String body = Encryption::encrypt(comEx.status_pins());
        client.print(body);
        client.flush();
      }
    }
    if (PIN::changed_flags) {
      Serial.println("[LOG] отправка изменений серверу");
      String body = Encryption::encrypt(comEx.status_pins());
      client.print(body);
      client.flush();
    }
    if (countProcess > 15000) {
      if (client.connected() && serverOK) {
        countProcess = 0;
        serverOK = false;
        String body = Encryption::encrypt(comEx.status_pins());
        client.print(body);
        client.flush();
      } else {
        countProcess = 0;
        Serial.println("[ERR] Мы не подключены к серверу");
        Serial.println("[LOG] Попытка повторной авторизации");
        client.stop();
        Internet::connect();
      }
    }
  }
}

char* Internet::read_buffer(size_t& buffer_size) {
  size_t maxSize = 4800;
  buffer_size = 0;
  char* buffer = new char[maxSize + 1];
  unsigned long start = millis();
  const unsigned long timeout = 200;  // максимум 0.5 сек

  while (millis() - start < timeout && buffer_size < maxSize) {
    while (client.available() > 0 && buffer_size < maxSize) {
      buffer[buffer_size++] = client.read();
      start = millis();
    }
    yield();
  }
  // Если клиент всё ещё шлёт данные — превышен лимит
  if (client.available()) {
    // очистка входного буфера
    while (client.available()) {
      client.read();
      yield();
    }
    buffer_size = 0;
    delete[] buffer;
    return nullptr;
  }
  buffer[buffer_size] = '\0';
  return buffer;
}
