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
#include "globals.h"

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
  delay(3000);
  if (client.connect("176.60.208.46", 64530)) {
    Serial.println("✅ Соединение с сервером установлено");
    client.print(body());
    client.flush();
    unsigned long start = millis();
    const unsigned long timeout = 15000;  // максимум 5 сек ожидания
    while (true) {
      if (client.available()) {
        int command = get_command();
        if (command < 0) {
          delay(100);
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
      }
      if (millis() - start > timeout) {
        Serial.println("⚠️ Таймаут ожидания ответа от сервера");
        break;
      }
      delay(5);
      // Serial.println("⚠️ Бесконечный цикл");
    }
    Serial.println("❌ Не удалось подключиться к серверу -1");
    return false;
  } else {
    Serial.println("❌ Не удалось подключиться к серверу -2");
    delay(10000);
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

uint32_t id_cmd{ 1 };
void Internet::processServerResponse() {

  CommandExecutor comEx;

  auto full_status = [&]() {
    auto body = Encryption::encrypt(comEx.full_status_json());
    to_send(body);
    PIN::changed_flags = false;
    PIR_SENSOR::changed_flags = false;
  };

  while (true) {
    ++countProcess;
    delay(5);
    if (client.available()) {

      String packet = read_buffer();
      if (packet.length() < 5) {  // 4 бита на ID и хоть что то еще должно быть
        continue;
      }
      String cmdId = packet.substring(0, 4);
      packet.remove(0, 4);
      String decrypted = Encryption::decrypt(packet);  // <-- передать сюда

      Serial.println("[LOG] получена команда -> " + decrypted);

      int comm = comEx.begin(decrypted);
      int err{};
      if (comm == Skeleton::idSer) {
        Serial.println("[INF] Ping <-> Pong");
        serverOK = true;
      }
      if (comm == Skeleton::pir) {
        err = comEx.playPIR(decrypted);
        serverOK = true;
      }
      if (comm == Skeleton::sm) {
        err = comEx.playSM(decrypted);
        serverOK = true;
      }
      if (comm == Skeleton::status_full) {
        serverOK = true;
        full_status();
      }
      // без указания ID
      if (comm >= Skeleton::pin2 && comm <= Skeleton::pin35) {
        err = comEx.playPIN(decrypted, comm);
      }
      // новый формат команды с ID
      else if (comm >= Skeleton::pin1) {
        auto idStirng = decrypted.substring(4, 8);
        unsigned int id = idStirng.toInt();
        for (int i{}; i < pinsG.size(); ++i) {
          if (id == pinsG[i].get_id()) {
            decrypted.remove(4, 4);
            err = comEx.playPIN(decrypted, id);
            break;
          }
        }
      }
      if (err == 0) {
        Serial.println("[LOG] Команда обработана без ошибок");
        String body = Encryption::encrypt(Skeleton::commands[Skeleton::accepted]);
        to_send(body, cmdId);
        delay(10);
      } else {
        Serial.print("[ERROR] Получена ошибка -> ");
        Serial.println(err);
      }
    }
    if (PIN::changed_flags || PIR_SENSOR::changed_flags) {
      Serial.println("[LOG] Отправка изменений серверу");
      full_status();
      go_fullstatus = millis();
    }
    else
    {
      if(millis() - go_fullstatus > interval)
      {
        go_fullstatus = millis();
        full_status();
      }
    }
    if (countProcess > 20000) {
      if (client.connected() && serverOK) {
        countProcess = 0;
        serverOK = false;
        String body = Encryption::encrypt(Skeleton::commands[Skeleton::idESP]);
        to_send(body);
      } else {
        countProcess = 0;
        Serial.println("[ERR] Мы не подключены к серверу");
        Serial.println("[LOG] Попытка повторной авторизации");
        while (!Internet::connect()) {
          delay(10);
        }
      }
    }
  }
}
char* Internet::read_buffer(size_t& buffer_size) {
  size_t maxSize = 4800;
  buffer_size = 0;
  char* buffer = new char[maxSize + 1];
  unsigned long start = millis();
  const unsigned long timeout = 100;  // максимум 0.5 сек

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
void Internet::to_send(const String& body, const String& id_) {

  String id{};
  if (id_ == "") {
    id = "abcd";
  } else {
    id = id_;
  }
  auto s = body.length();
  String second = String(s);
  auto size = second.length();
  if (size > 9) {
    Serial.print("[ERR] Не верный размер пакета для отправки -> ");
    Serial.println(s);
    return;
  }
  String first = String(size);

  String full_body = Skeleton::commands[Skeleton::start] + first + second + id + body;

  Serial.print("[LOG] Отпарвили  -> ");
  Serial.print(full_body.length());
  Serial.println(" байт");

  client.write((const uint8_t*)full_body.c_str(), full_body.length());  // отправка байт
  client.flush();
}
String Internet::read_buffer() {

  String body;
  body.reserve(8);
  unsigned long start = millis();
  const unsigned long timeout = 100;  // максимум 0.5 сек
  uint32_t pktSize{};
  int value{ -50 };
  bool read{ false };
  while (millis() - start < timeout && body.length() < 4500) {
    while (client.available() > 0 && body.length() < 4500) {
      char z = client.read();
      body.concat(z);
      start = millis();
      if (!read && body.length() == 4 && body != Skeleton::commands[Skeleton::start]) {
        body.remove(0, 1);
        continue;
      }
      if (!read && body.length() == 5) {
        char s = body[4];
        if (s < '0' || s > '9') {
          return {};
        }
        value = s - '0';
      }
      if (!read && body.length() == 5 + value) {
        String sizeStr = body.substring(5, 5 + value);  // берём следующие value символов
        int size = sizeStr.toInt();
        // проверка: сравниваем исходную строку с числом
        String check = String(size);
        if (check != sizeStr) {
          // ошибка: в sizeStr были недопустимые символы
          return {};
        }
        pktSize = size + 4;  // доп данные которые содержат ID
        if (pktSize > 4500) {
          return {};
        }
        body = "";
        body.reserve(pktSize);
        read = true;
        continue;
      }
      if (read && body.length() == pktSize) {
        return body;
      }
    }
  }
  Serial.println("[ERR] ❌ Пакет не прошел проверку сработал тайм аут");
  return {};
}