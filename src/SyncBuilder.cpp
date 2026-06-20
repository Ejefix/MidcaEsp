#include "SyncBuilder.h"
#include <algorithm>
#include "globals.h"
uint32_t ClientStreamSession::cmd_id_count{1};

void SyncBuilder::buildPINsSnapshot(String &out) const
{
    out.clear();

    JsonDocument mainDoc{};
    mainDoc["ID"] = Skeleton::id;
    mainDoc["time"] = millis();
    JsonObject data = mainDoc["data"].to<JsonObject>();

    JsonArray pinsJson = data["PINS"].to<JsonArray>();
    for (size_t i{}; i < pinsG.size(); ++i)
    {
        pinsG[i]->fill_json(pinsJson);
    }
    serializeJson(mainDoc, out); // строка для передачи
    size_t size = strlen(out.c_str());
    // Serial.print("[INF] Размер данных JSON PINs: ");
    //  Serial.print(size);
    // Serial.println(" байт");
    out = Skeleton::commands[Skeleton::snapshot_pins] + out;
}

void SyncBuilder::buildDeviceSnapshot(String &out) const
{
    out.clear();
    JsonDocument mainDoc{};
    mainDoc["ID"] = Skeleton::id;
    mainDoc["time"] = millis();
    JsonObject data = mainDoc["data"].to<JsonObject>();

    JsonArray deviceJson = data["DEVICE"].to<JsonArray>();
    device_registry->fill_json(deviceJson);

    serializeJson(mainDoc, out); // строка для передачи
    size_t size = strlen(out.c_str());
    // Serial.print("[INF] Размер данных JSON девайсов: ");
    //  Serial.print(size);
    // Serial.println(" байт");
    out = Skeleton::commands[Skeleton::snapshot_device] + out;
}

void SyncBuilder::buildIntentSnapshot(String &out) const
{
    out.clear();
    JsonDocument mainDoc{};
    mainDoc["ID"] = Skeleton::id;
    mainDoc["time"] = millis();
    JsonObject data = mainDoc["data"].to<JsonObject>();
    JsonArray storeJson = data["STORE"].to<JsonArray>();
    store->fill_json(storeJson);

    serializeJson(mainDoc, out); // строка для передачи
    size_t size = strlen(out.c_str());
    // Serial.print("[INF] Размер данных JSON магазина: ");
    //  Serial.print(size);
    // Serial.println(" байт");
    out = Skeleton::commands[Skeleton::snapshot_store] + out;
}

void SyncBuilder::buildConnectSnapshot(String &out) const
{
    out.clear();
    JsonDocument mainDoc{};
    mainDoc["ID"] = Skeleton::id;
    mainDoc["time"] = millis();
    JsonObject data = mainDoc["data"].to<JsonObject>();
    JsonArray connectJson = data["CONNECT"].to<JsonArray>();
    device_binder->fill_json(connectJson);

    serializeJson(mainDoc, out); // строка для передачи
    size_t size = strlen(out.c_str());
    // Serial.print("[INF] Размер данных JSON соединений: ");
    //  Serial.print(size);
    // Serial.println(" байт");
    out = Skeleton::commands[Skeleton::snapshot_connect] + out;
}

void SyncBuilder::buildPINsSnapshot(PinId id, String &out) const
{
    out.clear();
    for (size_t i{}; i < pinsG.size(); ++i)
    {
        if (pinsG[i]->get_id() == id)
        {
            JsonDocument mainDoc{};
            mainDoc["ID"] = Skeleton::id;
            mainDoc["time"] = millis();
            JsonObject data = mainDoc["data"].to<JsonObject>();
            JsonArray pinsJson = data["PINS"].to<JsonArray>();
            pinsG[i]->fill_json(pinsJson);
            serializeJson(mainDoc, out); // строка для передачи
            size_t size = strlen(out.c_str());
            // Serial.print("[INF] Размер данных JSON PIN: ");
            //  Serial.print(size);
            // Serial.println(" байт");
            out = Skeleton::commands[Skeleton::snapshot_pins] + out;
            return;
        }
    }
}

void SyncBuilder::buildIntentSnapshot(ScheduledIntentID id, String &out) const
{
    out.clear();
    auto intent = store->get(id);
    if (!intent)
        return;
    JsonDocument mainDoc{};
    mainDoc["ID"] = Skeleton::id;
    mainDoc["time"] = millis();
    JsonObject data = mainDoc["data"].to<JsonObject>();
    JsonArray intentJson = data["INTENT"].to<JsonArray>();
    intent->fill_json(intentJson);
    serializeJson(mainDoc, out); // строка для передачи
    size_t size = strlen(out.c_str());
    // Serial.print("[INF] Размер данных JSON: ");
    //  Serial.print(size);
    // Serial.println(" байт");
    out = Skeleton::commands[Skeleton::snapshot_intent] + out;
}

void SyncBuilder::buildDeviceSnapshot(uint16_t id, String &out) const
{
    out.clear();
    auto device = device_registry->get(id);
    if (!device)
        return;
    JsonDocument mainDoc{};
    mainDoc["ID"] = Skeleton::id;
    mainDoc["time"] = millis();
    JsonObject data = mainDoc["data"].to<JsonObject>();
    JsonArray deviceJson = data["DEVICE"].to<JsonArray>();
    device_registry->fill_json(id, deviceJson);
    serializeJson(mainDoc, out); // строка для передачи
    size_t size = strlen(out.c_str());
    // Serial.print("[INF] Размер данных JSON девайса: ");
    //  Serial.print(size);
    // Serial.println(" байт");
    out = Skeleton::commands[Skeleton::snapshot_device] + out;
}

ClientTCP::ClientTCP(WiFiClient &&client, CLOCK &myclock, bool isAuth)
    : isAuth{isAuth},  client{std::move(client)}, auth{this->client, myclock}, session{this->client}, receiver{this->client}
{

}

ClientTCP::~ClientTCP()
{
     client.stop();
}

bool ClientTCP::begin()
{

    if (isAuth)
    {
        if (auth.authorize())
        {
            session.begin();
            receiver.begin();
            return true;
        }
    }
    else
    {
        if (!client.connected())
            return false;
        session.begin();
        receiver.begin();
        return true;
    }

    return false;
}

bool ClientTCP::isConnected()
{
    return client.connected();
}

void ClientTCP::set_isAuth(bool isAuth)
{
    this->isAuth = isAuth;
}

void ClientTCP::set_adr(String adr)
{
    auth.set_adr(adr);
}

void ClientTCP::set_port(uint16_t port)
{
     auth.set_port(port);
}

ClientStreamSession::ClientStreamSession(WiFiClient &client_) : client(client_)
{
}

void ClientStreamSession::begin()
{

    sendUpdatePins();
    sendUpdateDevice();
    sendUpdateStore();
    sendUpdateConnect();
    if (!buffer.empty() && millis() - time_send > 30)
    {
        to_send(client, buffer.front());
        buffer.pop_front();
        time_send = millis();
    }
    
}

void ClientStreamSession::reset()
{

    versionIntent.clear();
    versionPINS.clear();
    versionDevice.clear();
    versionStore = 0;
    versionDevice_registry = 0;
    versionDevice_bind = 0;
}
uint32_t counter = 0;
size_t last_mb = -1;
void ClientStreamSession::to_send(WiFiClient &client_, const String &body)
{

    counter += body.length();
    if ((counter >> 20) != last_mb)
    {
        last_mb = (counter >> 20);

        double mb = counter / 1024.0 / 1024.0;
        Serial.print("[LOG] Отправлено всего -> ");
        Serial.print(mb, 3);
        Serial.println(" MB");
    }

    client_.write((const uint8_t *)body.c_str(), body.length()); // отправка байт
    client_.flush();
}
String ClientStreamSession::fullBody(const String &body, const String &id_)
{
    String id{};
    if (id_ == "" || id_.length() != 4)
    {
        char buf[4];
        buf[0] = (cmd_id_count >> 24) & 0xFF;
        buf[1] = (cmd_id_count >> 16) & 0xFF;
        buf[2] = (cmd_id_count >> 8) & 0xFF;
        buf[3] = cmd_id_count & 0xFF;
        ++cmd_id_count;
        id = String(buf, 4);
    }
    else
    {
        id = id_;
    }
    // Serial.print("[ClientStreamSession] ID cmd -> ");
    // Serial.println(id);
    auto s = body.length();      // получаем размер тела
    String second = String(s);   // переделываем в текст
    auto size = second.length(); // получаем количество символов
    if (size > 9)
    {
        Serial.print("[ERR] Не верный размер пакета для отправки -> ");
        Serial.println(s);
        return {};
    }
    String first = String(size); // 1 байт для того что бы понимали сколько символов размер данных

    // стартовая                          [count size]  [size data]  [id 4 байта]  [data]
    return Skeleton::commands[Skeleton::start] + first + second + id + body;
}
/*
void ClientStreamSession::sendFullStatus()
{
    String out;
    builder.buildDeviceSnapshot(out);

    to_send(client, enc.encrypt(out));
    versionDevice_registry = device_registry->get_version();
    auto list_idDevice = device_registry->get_ids();
    for (size_t i{}; i < list_idDevice.size(); ++i)
    {
        versionDevice[list_idDevice[i]] = device_registry->get_version(list_idDevice[i]);
    }
    builder.buildPINsSnapshot(out);
    to_send(client, enc.encrypt(out));
    for (size_t i{}; i < pinsG.size(); ++i)
    {
        versionPINS[pinsG[i]->get_id()] = pinsG[i]->get_version();
    }

    auto list_id = store->get_list_id();
    builder.buildIntentSnapshot(out);
    to_send(client, enc.encrypt(out));
    for (const auto &id : list_id)
    {
        versionIntent[id] = store->get_version(id);
    }
}
*/
void ClientStreamSession::sendUpdatePins()
{
    for (size_t i{}; i < pinsG.size(); ++i)
    {
        auto version = pinsG[i]->get_version();
        auto id = pinsG[i]->get_id();
        if (versionPINS.find(id) == versionPINS.end() || versionPINS[id] != version)
        {
            versionPINS[id] = version;
            String out;
            out.reserve(200); // резервируем память для строки
            builder.buildPINsSnapshot(id, out);
            buffer.push_back(fullBody(enc.encrypt(out)));
        }
    }
}

void ClientStreamSession::sendUpdateDevice()
{

    if (versionDevice_registry != device_registry->get_version())
    {
        versionDevice_registry = device_registry->get_version();
        auto list_idDevice = device_registry->get_ids();
        for (size_t i{}; i < list_idDevice.size(); ++i)
        {
            auto version = device_registry->get_version(list_idDevice[i]);
            if (versionDevice.find(list_idDevice[i]) == versionDevice.end() || versionDevice[list_idDevice[i]] != version)
            {
                String out;
                out.reserve(200); // резервируем память для строки
                versionDevice[list_idDevice[i]] = version;
                builder.buildDeviceSnapshot(list_idDevice[i], out);
                buffer.push_back(fullBody(enc.encrypt(out)));
            }
        }
    }
}

void ClientStreamSession::sendUpdateStore()
{
    if (versionStore != store->get_version())
    {

        auto list_id = store->get_list_id();
        // Serial.print("[ClientStreamSession] Версия магазина ");
        //  Serial.println(versionStore);
        for (auto it = versionIntent.begin(); it != versionIntent.end();)
        {
            auto exists = std::any_of(list_id.begin(), list_id.end(),
                                      [&](const ScheduledIntentID &id)
                                      {
                                          return id == it->first;
                                      });

            if (!exists)
            {
                it = versionIntent.erase(it);
            }
            else
            {
                ++it;
            }
        }
        for (const auto &id : list_id)
        {
            auto version = store->get_version(id);
            if (versionIntent.find(id) == versionIntent.end() || versionIntent[id] != version)
            {
                String out;
                out.reserve(200); // резервируем память для строки
                builder.buildIntentSnapshot(id, out);
                buffer.push_back(fullBody(enc.encrypt(out)));
                versionIntent[id] = version;
                //  Serial.print("[ClientStreamSession] Интент id  ");
                //  Serial.print(id);
                //  Serial.print(" версии ");
                //  Serial.print(version);
                //  Serial.println(" отправлен.");
            }
        }
        versionStore = store->get_version();
    }
}

void ClientStreamSession::sendUpdateConnect()
{
    if (versionDevice_bind != device_binder->get_version())
    {
        versionDevice_bind = device_binder->get_version();
        String out;
        out.reserve(200); // резервируем память для строки
        builder.buildConnectSnapshot(out);
        buffer.push_back(fullBody(enc.encrypt(out)));
    }
}

std::deque<String> ClientStreamReceiver::history{};
bool ClientStreamReceiver::isCommandProcessed(const String &id)
{
    return std::find(history.begin(), history.end(), id) != history.end();
}
void ClientStreamReceiver::parseIntent(const String &jsonStr)
{
    JsonDocument doc;
    auto error = deserializeJson(doc, jsonStr);
    if (error)
    {
        Serial.print("[ClientStreamReceiver::parseIntent] Ошибка парсинга JSON: ");
        Serial.println(error.c_str());
        return;
    }

    if (doc["ID"].isNull())
    {
        Serial.println("[ClientStreamReceiver::parseIntent] Ошибка: JSON не содержит ключ 'ID'");
        return;
    }
    String id = doc["ID"].as<String>();
    if (id != Skeleton::id)
    {
        Serial.println("[ClientStreamReceiver::parseIntent] Ошибка: ID в JSON не совпадает с ID устройства, адресс не верный");
        return;
    }
    if (doc["data"].isNull() || !doc["data"].is<JsonObject>())
    {
        Serial.println("[ClientStreamReceiver::parseIntent] Ошибка: JSON не содержит ключ 'data' или он не является объектом");
        return;
    }
    JsonObject data = doc["data"].as<JsonObject>();
    if (data["INTENT"].isNull() || !data["INTENT"].is<JsonArray>())
    {
        Serial.println("[ClientStreamReceiver::parseIntent] Ошибка: JSON не содержит ключ 'INTENT' или он не является массивом");
        return;
    }
    JsonArray intentJson = data["INTENT"].as<JsonArray>();
    for (size_t i{}; i < intentJson.size(); ++i)
    {
        JsonObject intentObj = intentJson[i].as<JsonObject>();
        ScheduledIntent intent;
        if (!intent.fill_from_json(intentObj))
        {
            Serial.println("[ClientStreamReceiver::parseIntent] Ошибка: Неверный формат данных интента в JSON");
            continue;
        }
        else
        {
            Serial.println("[ClientStreamReceiver::parseIntent] Интент успешно распарсен из JSON, добавляем в магазин");
            store->add(intent);
        }
    }
}
ClientStreamReceiver::ClientStreamReceiver(WiFiClient &client_) : client(client_)
{
}

void ClientStreamReceiver::begin()
{
    if (client.available())
    {
        Serial.println();
        Serial.println("[LOG] ------------- Сервер выслал данные. -----------");
        communication_socet();
        Serial.println("[LOG] ------------- Закончили с сервером  ----------- ");
        Serial.println();
    }
}

int ClientStreamReceiver::communication_socet()
{

    String packet = read_buffer();
    if (packet.length() < 5)
    { // 4 бита на ID и хоть что то еще должно быть
        return -1;
    }

    String cmdId = packet.substring(0, 4);
    Serial.print("[LOG] Принята команда ID ");
    Serial.println(cmdId);

    packet.remove(0, 4);
    packet = enc.decrypt(packet); // <-- передать сюда
    if (packet.isEmpty())
    {
        Serial.println("[LOG] Не удалось расшифровать");
        return -27;
    }

    Serial.println("[LOG] получена команда -> " + packet);
    if (packet.length() < 4)
        return -2;
    String command = packet.substring(0, 4);

    if (isCommandProcessed(cmdId))
    {
        Serial.println("[communication_socet] уже обрабатывали эту команду, пропускаем -> " + cmdId);
        return 0;
    }
    else
    {
        history.push_back(cmdId);
        while (history.size() > 100)
        {
            history.pop_front(); // удаляем самый старый ID, чтобы не допустить бесконечного роста в случае постоянного потока команд
        }
    }

    int com{-1};
    for (int i{}; i < Skeleton::end; ++i)
    {
        if (command == Skeleton::commands[i])
        {
            com = i;
            break;
        }
    }
    if (com == -1)
    {
        Serial.println("[ERR] Неизвестная команда -> " + command);
        return -3;
    }
    packet.remove(0, 4);
    switch (com)
    {
    case Skeleton::intent:
        Serial.println("[LOG] Команда INTENT");
        parseIntent(packet);
        break;
    case Skeleton::ping_pong:
        Serial.println("[LOG] Команда PING_PONG");
        break;
    default:
        Serial.println("[LOG] Команда " + command);
        Serial.println("[LOG] Данных для обработки нет");
        Serial.println(packet);
        break;
    }
    return 0;
}

String ClientStreamReceiver::read_buffer()
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
        while (client.available() > 0 && body.length() < 4500)
        {
            char z = client.read();
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
