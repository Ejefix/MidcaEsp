#include "SyncBuilder.h"
#include <algorithm>
#include "globals.h"
uint32_t SyncBuilder::cmd_id_count{1};

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

void SyncBuilder::buildSTORESnapshot(String &out) const
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

void SyncBuilder::buildPINsSnapshot(std::vector<PinId> ids, String &out) const
{
    out.clear();
    JsonDocument mainDoc{};
    mainDoc["ID"] = Skeleton::id;
    mainDoc["time"] = millis();
    JsonObject data = mainDoc["data"].to<JsonObject>();
    JsonArray pinsJson = data["PINS"].to<JsonArray>();

    for (size_t z{}; z < ids.size(); ++z)
    {
        for (size_t i{}; i < pinsG.size(); ++i)
        {
            if (pinsG[i]->get_id() == ids[z])
            {
                pinsG[i]->fill_json(pinsJson);
                // size_t size = strlen(out.c_str());
                //  Serial.print("[INF] Размер данных JSON PIN: ");
                //   Serial.print(size);
                //  Serial.println(" байт");
            }
        }
    }
    serializeJson(mainDoc, out);
    out = Skeleton::commands[Skeleton::snapshot_pins] + out;
}

void SyncBuilder::buildIntentSnapshot(std::vector<ScheduledIntentID> id, String &out) const
{
    out.clear();

    std::vector<ScheduledIntent> intents{};
    for (size_t i{}; i < id.size(); ++i)
    {
        auto intent = store->get(id[i]);
        if (!intent)
            continue;
        intents.push_back(*intent);
    }

    JsonDocument mainDoc{};
    mainDoc["ID"] = Skeleton::id;
    mainDoc["time"] = millis();
    JsonObject data = mainDoc["data"].to<JsonObject>();
    JsonArray intentJson = data["INTENT"].to<JsonArray>();
    for (auto it = intents.begin(); it != intents.end(); ++it)
    {
        it->fill_json(intentJson);
    }

    serializeJson(mainDoc, out); // строка для передачи
    size_t size = strlen(out.c_str());

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
    out = Skeleton::commands[Skeleton::snapshot_device] + out;
}

ClientTCP::ClientTCP(WiFiClient &&client, CLOCK &myclock)
    : client{std::move(client)}, auth{this->client, myclock}, session{this->client}, receiver{this->client}
{
    client.setNoDelay(true);
    time_full_update = millis();
}

ClientTCP::~ClientTCP()
{
    client.stop();
}

bool ClientTCP::begin(const String &packet)
{
    auto start = millis();
    if (start - time_reset > 1000 * 60 * 3)
    {
        time_reset = time_full_update = start;
    }

    if (auth.authorize())
    {

        session.send(packet);
        if (!receiver.receive().isEmpty())
        {
            Serial.println("[ClientTCP::begin] ✅ Пакет получен");
        }
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

void ClientTCP::set_UDT_data(String &data)
{
    // receiver.communication_socet(data);
}
void SyncBuilder::reset()
{

    versionIntent.clear();
    versionPINS.clear();
    versionDevice.clear();
    // buffer.clear();
    // bufferIntent.clear();
    //  bufferPINS.clear();
    versionStore = 0;
    versionDevice_registry = 0;
    versionDevice_bind = 0;
}
String SyncBuilder::begin()
{
    auto now = millis();
    if (buffer.empty())
    {
        if (counter_buffer == 0)
            sendUpdatePins();
        if (counter_buffer == 1)
            sendUpdateDevice();
        if (counter_buffer == 2)
            sendUpdateStore();
        if (counter_buffer == 3)
            sendUpdateConnect();

        ++counter_buffer;
        if (counter_buffer > 3)
            counter_buffer = 0;
    }

    if (!buffer.empty())
    {
        String ret = std::move(buffer.front());
        buffer.pop_front();
        return ret;
    }
    return {};
}
void SyncBuilder::add_buffer(const String &out)
{
    buffer.push_back(fullBody(enc.encrypt(out)));
}

String SyncBuilder::fullBody(const String &body, const String &id_)
{
    if (body.isEmpty())
        return {};
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
    /*
        [START]
        [SIZE:2 bytes]
        [TARGET_ID:8 bytes]
        [COMMAND_ID:4 bytes]
        [BODY:SIZE - 12 bytes]
    */
    uint64_t deviceId = ESP.getEfuseMac();
    char buf[8];
    buf[0] = (deviceId >> 56) & 0xFF;
    buf[1] = (deviceId >> 48) & 0xFF;
    buf[2] = (deviceId >> 40) & 0xFF;
    buf[3] = (deviceId >> 32) & 0xFF;
    buf[4] = (deviceId >> 24) & 0xFF;
    buf[5] = (deviceId >> 16) & 0xFF;
    buf[6] = (deviceId >> 8) & 0xFF;
    buf[7] = deviceId & 0xFF;
    String packet = String(buf, 8);
    packet += id;
    packet += body;
    uint16_t tailSize = packet.length();
    char size[2];
    size[0] = (tailSize >> 8) & 0xFF;
    size[1] = tailSize & 0xFF;

    return Skeleton::commands[Skeleton::start] + String(size, 2) + packet;

    // Skeleton::commands[Skeleton::start]  + [size data 2 байта — размер хвоста ]  + [ хвост [ 8 байт — Target ID ] + [4 байта — Command ID] + [Body] ]
}

void SyncBuilder::sendUpdatePins()
{

    if (millis() - last_PINS < 100)
    {
        return;
    }
    last_PINS = millis();
    std::vector<PinId> ids{};
    for (size_t i{}; i < pinsG.size(); ++i)
    {
        auto version = pinsG[i]->get_version();
        auto id = pinsG[i]->get_id();
        if (versionPINS.find(id) == versionPINS.end() || versionPINS[id] != version)
        {

            ids.push_back(id);
            versionPINS[id] = version;
            if (ids.size() >= 5)
            {
                String out;
                buildPINsSnapshot(ids, out);
                if (!out.isEmpty())
                {
                    buffer.push_back(fullBody(enc.encrypt(out)));
                    ids.clear();
                }
            }
        }
    }
    if (!ids.empty())
    {
        String out;
        buildPINsSnapshot(ids, out);
        if (!out.isEmpty())
        {
            buffer.push_back(fullBody(enc.encrypt(out)));
        }
    }
}

void SyncBuilder::sendUpdateDevice()
{

    if (millis() - last_Device < 100)
    {
        return;
    }
    last_Device = millis();
    if (versionDevice_registry != device_registry->get_version())
    {

        auto list_idDevice = device_registry->get_ids();
        for (size_t i{}; i < list_idDevice.size(); ++i)
        {
            auto version = device_registry->get_version(list_idDevice[i]);
            if (versionDevice.find(list_idDevice[i]) == versionDevice.end() || versionDevice[list_idDevice[i]] != version)
            {
                String out;
                out.reserve(200); // резервируем память для строки
                versionDevice[list_idDevice[i]] = version;
                buildDeviceSnapshot(list_idDevice[i], out);
                if (!out.isEmpty())
                {
                    buffer.push_back(fullBody(enc.encrypt(out)));
                }
            }
        }
        versionDevice_registry = device_registry->get_version();
    }
}

void SyncBuilder::sendUpdateStore()
{

    if (millis() - last_Store < 100)
    {
        return;
    }
    last_Store = millis();
    if (versionStore != store->get_version())
    {
        auto list_id = store->get_list_id();

        //  чистим наш писок версий и удаляем те что уже нету
        controlversionIntent(list_id);
        std::vector<ScheduledIntentID> ids{};
        for (const auto &id : list_id)
        {
            auto version = store->get_version(id);
            if (versionIntent.find(id) == versionIntent.end() || versionIntent[id] != version)
            {
                ids.push_back(id);
                versionIntent[id] = version;
            }
            if (ids.size() > 4)
            {
                String out;
                out.reserve(500); // резервируем память для строки
                buildIntentSnapshot(ids, out);
                ids.clear();
                if (!out.isEmpty())
                {
                    buffer.push_back(fullBody(enc.encrypt(out)));
                }
            }
        }
        if (!ids.empty())
        {
            String out;
            out.reserve(500); // резервируем память для строки
            buildIntentSnapshot(ids, out);
            ids.clear();
            if (!out.isEmpty())
            {
                buffer.push_back(fullBody(enc.encrypt(out)));
            }
        }
    }
    versionStore = store->get_version();
}

void SyncBuilder::sendUpdateConnect()
{
    if (millis() - last_Connect < 100)
    {
        return;
    }
    last_Connect = millis();
    if (versionDevice_bind != device_binder->get_version())
    {
        versionDevice_bind = device_binder->get_version();
        String out;
        out.reserve(200); // резервируем память для строки
        buildConnectSnapshot(out);
        if (!out.isEmpty())
        {
            buffer.push_back(fullBody(enc.encrypt(out)));
        }
    }
}

void SyncBuilder::controlversionIntent(const std::vector<ScheduledIntentID> &actual)
{
    for (auto it = versionIntent.begin(); it != versionIntent.end();)
    {
        auto exists = std::any_of(actual.begin(), actual.end(),
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

int ClientStreamReceiver::begin()
{
    static uint32_t last_print = 0;
    static uint32_t max_time = 0;
    auto now = millis();

    int answer{-1};
    if (client.available())
    {

        answer = communication_socet();
    }
    if (millis() - last_print > 10000)
    {
        auto current_time = millis() - now;
        if (current_time > max_time)
        {
            max_time = current_time;
        }
        last_print = millis();
        Serial.print("[INFO time] Время выполнения  ClientStreamReceiver = ");
        Serial.print(current_time);
        Serial.print("  max_time = ");
        Serial.println(max_time);
    }
    return answer;
}

int ClientStreamReceiver::communication_socet(String &packet)
{
    if (packet.length() < 5)
    { // 4 бита на ID и хоть что то еще должно быть
        return -1;
    }

    char s = packet[4];
    if (s < '0' || s > '9')
    {
        Serial.println("[ERR] ❌ value не цифра");
        return {};
    }
    int value = s - '0';
    packet.remove(0, 5 + value);

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
        // Serial.println("[LOG] Команда INTENT");
        parseIntent(packet);
        return Skeleton::intent;
    case Skeleton::ping_pong:
        //  Serial.println("[LOG] Команда PING_PONG");
        return Skeleton::ping_pong;
    default:
        Serial.println("[LOG] Команда " + command);
        break;
    }
    return 0;
}

int ClientStreamReceiver::communication_socet()
{

    String packet = read_buffer();
    if (packet.length() < 5)
    { // 4 бита на ID и хоть что то еще должно быть
        return -1;
    }

    String cmdId = packet.substring(0, 4);
    // Serial.print("[LOG] Принята команда ID ");
    // Serial.println(cmdId);

    packet.remove(0, 4);
    packet = enc.decrypt(packet); // <-- передать сюда
    if (packet.isEmpty())
    {
        Serial.println("[LOG] Не удалось расшифровать");
        return -27;
    }

    // Serial.println("[LOG] получена команда -> " + packet);
    if (packet.length() < 4)
        return -2;
    String command = packet.substring(0, 4);

    if (isCommandProcessed(cmdId))
    {
        // Serial.println("[communication_socet] уже обрабатывали эту команду, пропускаем -> " + cmdId);
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
        // Serial.println("[LOG] Команда INTENT");
        parseIntent(packet);
        return Skeleton::intent;
    case Skeleton::ping_pong:
        //  Serial.println("[LOG] Команда PING_PONG");
        return Skeleton::ping_pong;
    default:
        Serial.println("[LOG] Команда " + command);
        break;
    }
    return 0;
}

String ClientStreamReceiver::read_buffer()
{
    /*
    Структура пакета:
    [idESP 4 байта]         Проверка с Skeleton::commands[Skeleton::idESP]
    [idESP]                 ID ESP
    [start 4 байта]         Проверка с Skeleton::commands[Skeleton::start]
    [1 байт: value]         Кол-во символов, в которых записан размер payload
    [sizeStr (value)]       Число, размер payload + ID
    [ID 4 байта]            Идентификатор пакета
    [payload (size байт)]   Основные данные команды
    */

    String body;
    body.reserve(8);

    unsigned long start = millis();
    uint32_t pktSize{};
    uint32_t counter_size{};
    int value{-50};
    bool read{false};
    // if (!searhID())
    {
        //    Serial.println("[LOG] Получен пакет с неверным ID");
        //     return {};
    }
    while (millis() - start < timeout && body.length() < bodyMaxSize)
    {

        while (client.available() > 0 && body.length() < bodyMaxSize)
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
                // Serial.println("[LOG] Нашли начало пакета.");
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
                // Serial.print("[STATE] value = ");
                // Serial.println(value);
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

                // Serial.print("[STATE] pktSize = ");
                //  Serial.println(pktSize);

                body = "";
                body.reserve(pktSize);
                read = true;
                // Serial.println("[STATE] Начало чтения payload");
                continue;
            }

            // Чтение payload
            if (read && body.length() == pktSize)
            {
                // Serial.println("[LOG] Пакет полностью считан");

                //  String hex = "";
                // for (size_t i = 0; i < body.length(); i++)
                //   {
                //      char c = body[i];
                //      char buf[4];
                //      sprintf(buf, "%02X ", (unsigned char)c);
                //      hex += buf;
                //   }
                // Serial.println("[HEX] " + hex);

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

bool ClientStreamReceiver::searhID()
{
    String body;
    body.reserve(Skeleton::id.length());
    unsigned long start = millis();
    bool id{false};
    while (millis() - start < timeout)
    {
        while (client.available() > 0)
        {
            char z = client.read();
            body.concat(z);
            start = millis();
            if (!id && body.length() == 4 && body != Skeleton::commands[Skeleton::idESP])
            {
                body.remove(0, 1); // сдвиг окна на 1
                continue;
            }
            if (!id && body.length() == 4 && body == Skeleton::commands[Skeleton::idESP])
            {
                id = true;
                body.clear();
            }
            if (body.length() == Skeleton::id.length())
            {
                if (Skeleton::id == body)
                    return true;
                else
                {
                    body.clear();
                    id = false;
                }
            }
        }
    }
    return false;
}

TCPSender::TCPSender(WiFiClient &client) : client(client)
{
}

bool TCPSender::send(const String &packet)
{
    if (packet.isEmpty())
        return false;

    size_t packetSize = packet.length(); // размер текущего пакета

    client.write((const uint8_t *)packet.c_str(), packetSize); // отправка пакета
    client.flush();
    return true;
}

UDPSender::UDPSender(WiFiUDP &client, uint16_t portUDT) : client(client), portUDT(portUDT)
{
}

bool UDPSender::update_broadcast()
{
    IPAddress ip = WiFi.localIP();
    IPAddress subnet = WiFi.subnetMask();

    if (ip == IPAddress(0, 0, 0, 0))
        return false;

    if (subnet == IPAddress(0, 0, 0, 0))
        return false;

    for (int i = 0; i < 4; ++i)
        broadcast[i] = ip[i] | ~subnet[i];

    // Serial.print("Broadcast: ");
    // Serial.println(broadcast);
    return true;
}

bool UDPSender::send(const String &packet)
{
    if (packet.isEmpty())
    {
        return false;
    }

    const size_t total = packet.length();

    // Serial.print("[UDP] Хотим отправить: ");
    // Serial.print(total);
    //  Serial.println(" байт");

    if (!client.beginPacket(broadcast, portUDT))
    {
        Serial.println("[UDP] ❌ beginPacket() failed");
        return false;
    }

    const size_t written = client.write(
        reinterpret_cast<const uint8_t *>(packet.c_str()),
        total);

    // Serial.print("[UDP] write(): ");
    // Serial.print(written);
    // Serial.print(" / ");
    // Serial.print(total);
    //  Serial.println(" байт");

    if (written != total)
    {
        Serial.print("[UDP] ❌ Не удалось записать: ");
        Serial.print(total - written);
        Serial.println(" байт");
    }

    const int result = client.endPacket();

    // Serial.print("[UDP] endPacket(): ");
    // Serial.println(result);

    if (result != 1)
    {
        Serial.println("[UDP] ❌ Пакет не отправлен");
        return false;
    }

    // Serial.println("[UDP] ✅ Пакет отправлен");

    return written == total;
}

void UDPSender::set_portUDT(uint16_t portUDT)
{
    this->portUDT = portUDT;
}

NetworkManager::NetworkManager(CLOCK &myclock) : serwer{std::move(WiFiClient{}), myclock}, udpSender{Udp, 1002}, udpReceiver{UdpReceiver}
{
}

bool NetworkManager::update_setup()
{

    UdpReceiver.begin(1005);
    return udpSender.update_broadcast();
}

bool NetworkManager::begin()
{

    const String packet{builder.begin()};
    serwer.begin(packet);
    if (udpSender.update_broadcast())
    {

        udpSender.send(packet);
    }

    auto text = udpReceiver.receive();
    if (!text.isEmpty())
    {
        Serial.println("[NetworkManager::begin] Пакет получен");
        parseIntent(text);
    }
    return true;
}

void NetworkManager::reset()
{
    builder.reset();
}

void NetworkManager::parseIntent(String &packet)
{
    if (packet.length() < 19)
        return;
    packet.remove(0, 14);
    String id = packet.substring(0, 4);
    Serial.print("[NetworkManager::parseIntent] Найдено ID : ");
    Serial.println(id);
    packet.remove(0, 4);
    Encryption enc{};
    packet = enc.decrypt(packet); // <-- передать сюда
    if (packet.isEmpty())
    {
        Serial.println("[NetworkManager::parseIntent] Не удалось расшифровать");
        return;
    }
    if (packet.length() < 4)
        return;
    String command = packet.substring(0, 4);

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
        return;
    }
    packet.remove(0, 4);
    switch (com)
    {
    case Skeleton::intent:
    {
        Serial.println("[LOG] Команда INTENT");
        JsonDocument doc;
        auto error = deserializeJson(doc, packet);
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
        return;
    }
    case Skeleton::ping_pong:
        //  Serial.println("[LOG] Команда PING_PONG");
        return;
    default:

        break;
    }
}

TCPReceiver::TCPReceiver(WiFiClient &client) : client{client}
{
}

bool TCPReceiver::hasData()
{
    return client.available() > 0;
}

int TCPReceiver::readByte()
{
    return client.read();
}

UDPReceiver::UDPReceiver(WiFiUDP &udp) : client{udp}
{
}

bool UDPReceiver::hasData()
{
    if (client.available() > 0) // Проверяем, есть ли уже доступные байты.
    {
        return true; // Байты есть, можно читать.
    }

    if (client.parsePacket() > 0) // Ищем новую UDP datagram.
    {
        return true; // Новая datagram найдена, теперь её можно читать.
    }

    return false; // Данных нет.
}

int UDPReceiver::readByte()
{

    return client.read();
}

String IReceiver::receive()
{

    return read_buffer();
}

String IReceiver::read_buffer()
{
    while (hasData()) // Пока транспорт предоставляет новые байты.
    {
        const int value = readByte(); // Читаем один байт.

        rxBuffer += static_cast<char>(value); // Добавляем байт в буфер.

        if (value < 0) // Проверяем ошибку чтения.
        {
            return String{}; // Пакет пока не готов.
        }

        if (rxBuffer.length() >= bodyMaxSize) // Проверяем максимальный размер буфера.
        {
            rxBuffer.clear(); // Очищаем переполненный буфер.
            read = false;     // Сбрасываем состояние парсера.
            return String{};  // Пакет не готов.
        }

        if (!read) // Проверяем, обработан ли заголовок.
        {
            if (rxBuffer.length() < 14) // Проверяем наличие полного заголовка.
            {
                continue; // Продолжаем получать байты.
            }

            if (rxBuffer.substring(0, 4) != Skeleton::commands[Skeleton::start]) // Проверяем START.
            {
                rxBuffer.remove(0, 1); // Удаляем один байт для продолжения поиска START.
                continue;              // Продолжаем поиск START.
            }
            sizePacket = // Получаем размер хвоста пакета.
                (static_cast<uint16_t>(static_cast<uint8_t>(rxBuffer[4])) << 8) |
                static_cast<uint8_t>(rxBuffer[5]); // Получаем младший байт размера.
            Serial.print("[IReceiver::read_buffer] Размер пакета ");
            Serial.println(sizePacket);
            const uint64_t targetId = // Получаем ID получателя.
                (static_cast<uint64_t>(static_cast<uint8_t>(rxBuffer[6])) << 56) |
                (static_cast<uint64_t>(static_cast<uint8_t>(rxBuffer[7])) << 48) |
                (static_cast<uint64_t>(static_cast<uint8_t>(rxBuffer[8])) << 40) |
                (static_cast<uint64_t>(static_cast<uint8_t>(rxBuffer[9])) << 32) |
                (static_cast<uint64_t>(static_cast<uint8_t>(rxBuffer[10])) << 24) |
                (static_cast<uint64_t>(static_cast<uint8_t>(rxBuffer[11])) << 16) |
                (static_cast<uint64_t>(static_cast<uint8_t>(rxBuffer[12])) << 8) |
                static_cast<uint64_t>(static_cast<uint8_t>(rxBuffer[13])); // Получаем младший байт ID.
            Serial.print("[IReceiver::read_buffer] ID цели ");
            Serial.println(targetId);
            if (targetId != ESP.getEfuseMac()) // Проверяем ID получателя.
            {
                rxBuffer.remove(0, 14); // Удаляем заголовок чужого пакета.
                read = false;           // Оставляем парсер в состоянии поиска нового пакета.
                continue;               // Ищем следующий пакет.
            }

            read = true; // Запоминаем, что заголовок текущего пакета обработан.
        }

        const size_t packetSize = 6 + sizePacket; // Вычисляем полный размер пакета.

        if (rxBuffer.length() < packetSize) // Проверяем наличие всего пакета.
        {
            return String{}; // Пакет ещё не получен полностью.
        }

        String result = rxBuffer.substring(0, packetSize); // Копируем полностью полученный пакет.

        rxBuffer.remove(0, packetSize); // Удаляем готовый пакет из накопительного буфера.

        read = false; // Сбрасываем состояние для следующего пакета.

        return result; // Возвращаем полностью полученный пакет.
    }

    return String{}; // Если полный пакет не получен, возвращаем пустую строку.
}
