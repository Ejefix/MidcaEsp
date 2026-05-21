#include "DeviceFactory.h"
#include "globals.h"
#include "switch433.h"
bool DeviceRegistry::changed_flags{false};
IInputDevice *DeviceFactory::create(uint8_t mcp_id, DeviceType type, uint8_t pin_, uint8_t id)
{
    if (mcp_id >= gpioSignal.size())
    {
        return nullptr;
    }
    if (pin_ >= 16)
    {
        return nullptr;
    }

    switch (type)
    {
    case DeviceType::Sensor:
    {
        return new SENSOR(gpioSignal[mcp_id][pin_], id);
    }
    case DeviceType::Switch:
    {
        return new SwitchMechanics(gpioSignal[mcp_id][pin_]);
    }
    case DeviceType::Button:
    {
        return new SwitchButton(gpioSignal[mcp_id][pin_]);
    }
    }
    return nullptr;
}

DeviceRegistry::DeviceRegistry()
{
    devices.reserve(32);
}

bool DeviceRegistry::add(uint8_t mcp_id, DeviceType type, uint8_t pin)
{
    uint16_t id = make_id(mcp_id, pin);

    for (size_t i{}; i < devices.size(); ++i)
    {
        if (devices[i].id == id)
        {
            if (devices[i].type == type)
                return true;
            return false;
        }
    }
    IInputDevice *raw = DeviceFactory::create(mcp_id, type, pin, id);
    if (!raw)
        return false;

    devices.emplace_back(raw, type, pin, id);
    Serial.print("[DeviceRegistry] добавляем устройство с id ");
    Serial.println(id);
    return true;
}

bool DeviceRegistry::exists(uint16_t id) const
{
    return std::any_of(devices.begin(), devices.end(),
                       [id](const DeviceEntry &d)
                       {
                           return d.id == id;
                       });
}

std::vector<std::pair<uint16_t, DeviceType>> DeviceRegistry::get_ids() const
{
    std::vector<std::pair<uint16_t, DeviceType>> ids;

    for (const auto &dev : devices)
    {
        ids.push_back({dev.id, dev.type});
    }
    return ids;
}

bool DeviceRegistry::set_type(DeviceType type, uint16_t id)
{
    for (size_t i{}; i < devices.size(); ++i)
    {
        if (devices[i].id == id)
        {
            auto id_id = parse_id(id);
            if (devices[i].type == type)
            {
                return true;
            }
            IInputDevice *raw =
                DeviceFactory::create(id_id.first, type, devices[i].pin, id);
            if (!raw)
            {
                Serial.println("[DeviceRegistry] ОШИБКА: factory вернул NULL");
                return false;
            }
            devices[i].type = type;

            devices[i].device.reset(raw);
            save();
            DeviceRegistry::changed_flags = true;
            return true;
        }
    }
    Serial.println("[SET_TYPE] ID НЕ НАЙДЕН");
    return false;
}

IInputDevice *DeviceRegistry::get(uint16_t id)
{
    auto it = std::find_if(devices.begin(), devices.end(),
                           [id](const auto &d)
                           {
                               return d.id == id;
                           });

    if (it == devices.end())
        return nullptr;

    return it->device.get();
}

void DeviceRegistry::fill_json(JsonArray &arr) const
{
    for (const auto &d : devices)
    {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = d.id;
        obj["type"] = static_cast<uint8_t>(d.type);
        auto sensor_base = d.device.get();

        auto sensor = dynamic_cast<SENSOR *>(sensor_base);
        if (sensor)
        {
            JsonArray data = obj["data"].to<JsonArray>();
            sensor->fill_json(data);
        }
    }
}

void DeviceRegistry::save() const
{

    File file = SPIFFS.open(path, "w");
    if (!file)
        return;
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    fill_json(arr);
    serializeJson(doc, file);
    file.close();
    Serial.println("[DeviceRegistry] сохранение... ");
}

void DeviceRegistry::load()
{
    File file = SPIFFS.open(path, "r"); // открываем файл

    if (!file) // если нет файла
        return;

    JsonDocument doc;                                      // JSON документ
    DeserializationError err = deserializeJson(doc, file); // читаем JSON

    if (err) // если ошибка парсинга
        return;

    JsonArray arr = doc.as<JsonArray>(); // получаем массив
    devices.clear();                     // очищаем текущие данные
    for (JsonObject obj : arr)           // перебор записей
    {
        uint16_t id = obj["id"]; // читаем id
        auto id_id = parse_id(id);
        uint8_t t = obj["type"];
        DeviceType type = static_cast<DeviceType>(t);
        IInputDevice *raw = DeviceFactory::create(id_id.first, type, id_id.second, id);
        if (!raw)
            continue;
        devices.emplace_back(raw, type, id_id.second, id);
    }
    file.close(); // закрываем файл
    Serial.println("[DeviceRegistry] загрузка... ");
}

uint16_t DeviceRegistry::make_id(uint8_t mcp, uint8_t pin)
{
    return (mcp << 8) | pin;
}

std::pair<uint8_t, uint8_t> DeviceRegistry::parse_id(uint16_t id)
{
    uint8_t mcp = (id >> 8) & 0xFF; // старшие 8 бит
    uint8_t pin = id & 0xFF;        // младшие 8 бит

    return {mcp, pin};
}

DeviceRegistry::DeviceEntry::DeviceEntry(IInputDevice *dev, DeviceType t, uint8_t p, uint16_t id_) : device(dev), type(t), pin(p), id(id_)
{
}

DeviceBinder::DeviceBinder()
{
    data.reserve(20);
}

void DeviceBinder::connect(DeviceId device, PIN *obj)
{
    if (!obj)
        return;

    auto it = data.find(device);

    if (it != data.end())
    {
        for (const auto c : it->second)
        {
            if (c == obj)
                return;
        }
    }
    data[device].push_back(obj); // добавляем connection в список устройства
}

void DeviceBinder::disconnect(DeviceId device, PIN *obj)
{
    if (!obj)
        return;

    auto it = data.find(device);
    if (it == data.end())
        return;

    auto &vec = it->second;

    for (auto c = vec.begin(); c != vec.end();)
    {
        if (*c == obj)
            c = vec.erase(c);
        else
            ++c;
    }

    if (vec.empty())
        data.erase(it);
}

void DeviceBinder::disconnect(DeviceId device)
{

    data.erase(device);
}

void DeviceBinder::disconnect(PIN *obj)
{
    for (auto it = data.begin(); it != data.end();)
    {
        auto &vec = it->second;

        for (auto c = vec.begin(); c != vec.end();)
        {
            if (*c == obj)
            {
                c = vec.erase(c);
            }
            else
            {
                ++c;
            }
        }
        if (vec.empty())
        {
            it = data.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void DeviceBinder::disconnect()
{
    data.clear();
}

void DeviceBinder::begin() const
{
    std::unordered_map<PIN *, std::vector<Event>> mail;

    // делаем снимок железа
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        DeviceId deviceID = it->first;
        auto dev = device_registry->get(deviceID);
        Event ev{};
        ev.id = deviceID;
        ev.event = dev->event();
        ev.type = dev->type();
        const auto &vecPIN = it->second;
        for (auto pin = vecPIN.begin(); pin != vecPIN.end(); ++pin)
        {
            mail[*pin].push_back(ev);
        }
    }
    for (auto it = mail.begin(); it != mail.end(); ++it)
    {
        it->first->event(it->second);
    }
}
