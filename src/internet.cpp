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

Internet::Internet(CLOCK &myclock)
    : tcpServer(new WiFiServer{TCP_PORT}), serwer{std::move(WiFiClient{}), myclock, true}
{
}
// это нужно для первого подлючения к серверу и авторизации
bool Internet::connect()
{

  return serwer.begin();
}

void Internet::startTCPSerwer()
{

  tcpServer->begin();
  tcpServer->setNoDelay(true); // отключаем Nagle, для мгновенной отправки
  Serial.print("TCP-сервер запущен на порту ");
  Serial.println(TCP_PORT);
  Udp.begin(1001);
}

void Internet::processServerResponse()
{
  while (true)
  {
    vTaskDelay(5);
    serwer.begin();
    WiFiClient clientDevice = tcpServer->available();
    if (clientDevice && clients.size() < 15)
    {
      Serial.println("[Internet] Подлючился девайс");
      clients.emplace_back(new ClientTCP{std::move(clientDevice), myclock, false});
    }

    for (auto it = clients.begin(); it != clients.end();)
    {
      if (!(*it)->isConnected())
      {
        Serial.println("[Internet] Удаляем невалидного клиента");
        delete *it;
        it = clients.erase(it);
      }
      else
      {
        (*it)->begin();
        ++it;
      }
    }
    communication_udp();
    static uint32_t last_print = 0;
    if (millis() - last_print > 10000)
    {
      Serial.println("Поток TCP работает");
      last_print = millis();
    }
  }
}

#if FW_BUILD == 6
void Internet::processServerResponse()
{

  while (true)
  {
    vTaskDelay(2);
    communication_udp();
    WiFiClient clientDevice = tcpServer->available();
    if (clientDevice)
    {
      Serial.println("[LOG] Подлючился девайс");
      clients.push_back(new WiFiClient(clientDevice));
      full_status(clientDevice);
    }
    if (clients.size() > 15)
    {
      clients[0]->stop();
      delete clients[0];
      clients.erase(clients.begin());
    }
    int number{1};
    for (auto it = clients.begin(); it != clients.end(); ++number)
    {
      WiFiClient *client = *it;
      if (client == nullptr || !client->connected())
      {
        Serial.println("[LOG] Удаляем невалидного клиента");

        if (client)
        {
          client->stop();
          delete client;
        }
        it = clients.erase(it);
        continue;
      }

      if (client->available())
      {
        Serial.println();
        Serial.print("[LOG] ++++++++++++++++ Девайс ");
        Serial.print(number);
        Serial.println(" выслал данные. +++++++++++++");
        communication_socet(*client);
        Serial.println("[LOG] ++++++++++++++++ Закончили с девайсом +++++++++++++++++");
        Serial.println();
      }
      ++it;
    }
    if (client_server.available())
    {
      if (connect_server)
      {
        Serial.println();
        Serial.println("[LOG] ------------- Сервер выслал данные. -----------");

        serverOK = true;

        Serial.println("[LOG] ------------- Закончили с сервером  ----------- ");
        Serial.println();
      }
      else
      {
        // authorization();
      }
    }
    auto device_full = [&]()
    {
      if (!clients.empty())
      {
        Serial.println("[LOG] Отправка изменений Device");
        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
          WiFiClient *client = *it;
          full_status(*client);
        }
      }
    };
    {

      if (updateDATA)
      {
        device_full();
        Serial.println("[LOG] Отправка изменений серверу");
        full_status(client_server);
        go_fullstatus = millis();
      }
      else
      {
        if (millis() - go_fullstatus > interval)
        {
          device_full();
          go_fullstatus = millis();
          Serial.println("[LOG] Отправка изменений серверу");
          full_status(client_server);
        }
      }
    }
    // ping_pong();
    test.begin();
  }
}
char *Internet::read_buffer(size_t &buffer_size)
{
  size_t maxSize = 4800;
  buffer_size = 0;
  char *buffer = new char[maxSize + 1];
  unsigned long start = millis();
  const unsigned long timeout = 100; // максимум 0.5 сек

  while (millis() - start < timeout && buffer_size < maxSize)
  {
    while (client_server.available() > 0 && buffer_size < maxSize)
    {
      buffer[buffer_size++] = client_server.read();
      start = millis();
    }
    yield();
  }
  // Если клиент всё ещё шлёт данные — превышен лимит
  if (client_server.available())
  {
    // очистка входного буфера
    while (client_server.available())
    {
      client_server.read();
      yield();
    }
    buffer_size = 0;
    delete[] buffer;
    return nullptr;
  }
  buffer[buffer_size] = '\0';
  return buffer;
}
void Internet::ping_pong()
{

  if (millis() - time_ping_pong > interval_ping_pong)
  {
    if (client_server.connected() && serverOK)
    {
      time_ping_pong = millis();
      serverOK = false;
      String body = Encryption::encrypt(Skeleton::commands[Skeleton::idESP]);
      Serial.println("[LOG] отправка данных на сервер ping_pong");
      to_send(client_server, body);
    }
    else
    {
      if (!serverOK)
      {
        client_server.stop();
      }
      serverOK = Internet::connect();
      time_ping_pong = millis();
      Serial.println("[ERR] Мы не подключены к серверу");
      Serial.println("[LOG] Попытка повторной авторизации");
      connect_server = serverOK;
    }
  }
}
void Internet::full_status(WiFiClient &client_)
{

  CommandExecutor comEx;

  if (updateDATA || fullStatus.isEmpty())
  {
    fullStatus = Encryption::encrypt(comEx.full_status_json());
    to_send(client_, fullStatus);
    updateDATA = false;
  }
  else
  {
    to_send(client_, fullStatus);
  }
}
int Internet::communication_socet(WiFiClient &client_)
{

  String packet = read_buffer(client_);
  if (packet.length() < 5)
  { // 4 бита на ID и хоть что то еще должно быть
    return -1;
  }

  String cmdId = packet.substring(0, 4);
  Serial.print("[LOG] Принята команда ID ");
  Serial.println(cmdId);
  for (int i{}; i < id_cmd.size(); ++i)
  {
    if (id_cmd[i] == cmdId)
    {
      Serial.println("[LOG] команда с таким ID уже обработана ");
      return 0;
    }
  }

  if (id_cmd.size() > 30)
  {
    id_cmd.erase(id_cmd.begin());
  }
  if (cmdId != "eror")
  {
    id_cmd.push_back(cmdId);
  }

  packet.remove(0, 4);
  String decrypted = Encryption::decrypt(packet); // <-- передать сюда
  if (decrypted.isEmpty())
  {
    Serial.println("[LOG] Не удалось расшифровать");
    return -27;
  }

  Serial.println("[LOG] получена команда -> " + decrypted);

  int answer = process_cmd(decrypted);

  if (answer == Skeleton::status_full)
  {
    String body = Encryption::encrypt(Skeleton::commands[Skeleton::accepted]);
    to_send(client_, body, cmdId);
    Serial.print("[LOG] Отправка подтверждения принятия комманды ");
    Serial.println(cmdId);
    full_status(client_);
    return answer;
  }

  if (answer >= 0)
  {
    // Serial.println("[LOG] Команда обработана без ошибок");
    String body = Encryption::encrypt(Skeleton::commands[Skeleton::accepted]);
    to_send(client_, body, cmdId);
  }
  else
  {
    Serial.print("[ERROR] Получена ошибка -> ");
    Serial.println(answer);
  }
  return answer;
}
void Internet::to_send(WiFiClient &client_, const String &body, const String &id_)
{

  if (!client_)
  {
    Serial.println("[ERR] Не можем отправить данные -> nullptr");
    return;
  }
  String id{};
  if (id_ == "")
  {
    id = String(cmd_id_count);
    ++cmd_id_count;
    if (cmd_id_count > 9999 || cmd_id_count < 1000)
    {
      cmd_id_count = 1000;
    }
  }
  else
  {
    id = id_;
  }
  auto s = body.length();      // получаем размер тела
  String second = String(s);   // переделываем в текст
  auto size = second.length(); // получаем количество символов
  if (size > 9)
  {
    Serial.print("[ERR] Не верный размер пакета для отправки -> ");
    Serial.println(s);
    return;
  }
  String first = String(size); // 1 байт для того что бы понимали сколько символов размер данных

  // стартовая                          [count size]  [size data]  [id 4 байта]  [data]
  String full_body = Skeleton::commands[Skeleton::start] + first + second + id + body;

  // Serial.print("[LOG] Отправили  -> ");
  // Serial.print(full_body.length());
  // Serial.println(" байт");

  client_.write((const uint8_t *)full_body.c_str(), full_body.length()); // отправка байт
  client_.flush();
}
int Internet::process_cmd(String &decrypted)
{

  CommandExecutor comEx;
  int comm = comEx.begin(decrypted);
  int err{};
  if (comm == Skeleton::idSer)
  {
    Serial.println("[INF] Ping <-> Pong");
    return 0;
  }

  if (comm == Skeleton::status_full)
  {
    return Skeleton::status_full;
  }

  if (comm >= Skeleton::pin1)
  {
    auto idStirng = decrypted.substring(4, 8);
    unsigned int id = idStirng.toInt();
    decrypted.remove(4, 4);
    for (int i{}; i < pinsG.size(); ++i)
    {
      if (id == 9999 || id == pinsG[i]->get_id())
      {
        err = comEx.playPIN(decrypted, pinsG[i]->get_id());
      }
    }
  }
  return err;
}

String Internet::read_buffer(WiFiClient &client_)
{
  /*
  Структура пакета:
  [start 4 байта]        Проверка с Skeleton::commands[Skeleton::start]
  [1 байт: value]         Кол-во символов, в которых записан размер payload
  [sizeStr (value)]       Число, размер payload + ID
  [ID 4 байта]            Идентификатор пакета
  [payload (size байт)]   Основные данные команды
  */

  String body;
  body.reserve(8);

  unsigned long start = millis();
  const unsigned long timeout = 300; // 0.3 сек таймаут
  uint32_t pktSize{};
  uint32_t counter_size{};
  int value{-50};
  bool read{false};

  while (millis() - start < timeout && body.length() < 4500)
  {
    while (client_.available() > 0 && body.length() < 4500)
    {
      char z = client_.read();
      ++counter_size;
      body.concat(z);
      start = millis();
      // Проверка стартовой последовательности
      if (!read && body.length() == 4 && body != Skeleton::commands[Skeleton::start])
      {
        body.remove(0, 1); // сдвиг окна на 1
        continue;
      }
      if (body.length() == 4 && body == Skeleton::commands[Skeleton::start])
      {
        Serial.println("[LOG] Нашли начало пакета.");
      }
      // Считываем value
      if (!read && body.length() == 5)
      {
        char s = body[4];
        if (s < '0' || s > '9')
        {
          Serial.println("[ERR] ❌ value не цифра");
          return {};
        }
        value = s - '0';
        Serial.print("[STATE] value = ");
        Serial.println(value);
      }

      // Считываем sizeStr
      if (!read && body.length() == 5 + value)
      {
        String sizeStr = body.substring(5, 5 + value);
        int size = sizeStr.toInt();
        if (String(size) != sizeStr)
        {
          Serial.println("[ERR] ❌ sizeStr содержит недопустимые символы");
          return {};
        }
        pktSize = size + 4; // учитываем ID
        if (pktSize > 4500)
          return {};

        Serial.print("[STATE] pktSize = ");
        Serial.println(pktSize);

        body = "";
        body.reserve(pktSize);
        read = true;
        Serial.println("[STATE] Начало чтения payload");
        continue;
      }

      // Чтение payload
      if (read && body.length() == pktSize)
      {
        Serial.println("[LOG] Пакет полностью считан");

        String hex = "";
        for (size_t i = 0; i < body.length(); i++)
        {
          char c = body[i];
          char buf[4];
          sprintf(buf, "%02X ", (unsigned char)c);
          hex += buf;
        }
        Serial.println("[HEX] " + hex);

        return body;
      }
    }
  }

  Serial.println("[ERR] ❌ Ошибка размера данных");
  Serial.print("[LOG] Получили ");
  Serial.print(body.length());
  Serial.print(". Нужно было ");
  Serial.println(pktSize);
  Serial.println("[ERR] ❌ Пакет не прочитан (таймаут)");
  return {};
}
#endif
void Internet::communication_udp()

{
  int packetSize = Udp.parsePacket(); // проверяем, пришёл ли пакет
  if (packetSize)
  {
    if (packetSize > 50)
    {
      // читаем и отбрасываем
      char discard[packetSize];
      Udp.read(discard, packetSize);
      Serial.println("Принято слишком длинное сообщение — откидываем");
      return;
    }

    char buffer[50];
    int len = Udp.read(buffer, 50);
    buffer[len] = 0; // завершаем строку нулём

    auto receivedRequest = String(buffer); // сохраняем в переменную
    Serial.print("Принято: ");
    Serial.println(receivedRequest);
    if (receivedRequest == local_name + Skeleton::id)
    {
      auto ip = wifi.get_my_ip();
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());

      Udp.write((const uint8_t *)ip.c_str(), ip.length());
      Udp.endPacket();

      Serial.print("Отправили IP: ");
      Serial.println(ip);
    }
  }
}
