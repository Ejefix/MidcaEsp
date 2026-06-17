#pragma once
#include "IInputDevice.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <ArduinoJson.h>
using DeviceId = uint32_t;

class DeviceFactory
{
public:
    static IInputDevice *create(uint8_t mcp_id, DeviceType type, uint8_t pin_, uint16_t id);
};

class DeviceRegistry
{
public:
    DeviceRegistry();
    // возвращает ID девайса , если 0 = error
    bool add(uint8_t mcp_id, DeviceType type, uint8_t pin);
    bool exists(uint16_t id) const;
    std::vector<uint16_t> get_ids() const;

    bool set_type(DeviceType type, uint16_t id);
    uint32_t get_version() const;
    uint32_t get_version(uint16_t id) const;
    IInputDevice *get(uint16_t id);

    void fill_json(JsonArray &arr) const;
    void fill_json(uint16_t id,JsonArray &arr) const;
    void save() const;
    void load();

private:
    uint16_t make_id(uint8_t mcp, uint8_t pin);
    std::pair<uint8_t, uint8_t> parse_id(uint16_t id);
    struct DeviceEntry
    {
        DeviceEntry(IInputDevice *dev, DeviceType t, uint8_t p, uint16_t id_);
        std::unique_ptr<IInputDevice> device;
        DeviceType type;
        uint8_t pin;
        uint16_t id{};
        uint8_t id_mcp;
    };
    std::vector<DeviceEntry> devices;
    const String path{"/devices.json"};
    uint32_t version{1};
};

using PinId = uint16_t;
using ScheduledIntentID = uint16_t;
class DeviceBinder
{

public:
    DeviceBinder();
    // если не смогли создать вернет 0
    bool connect(DeviceId device, PinId obj);

    void disconnect(DeviceId device, PinId obj);
    void disconnect(DeviceId device);
    void disconnect(PinId obj);
    void disconnect();
    void fill_json(JsonArray &arr) const;
    // тут создаются намериния в зависимости от результата опроса девайса
    void begin();
    uint32_t get_version() const;
    void save() const;
    void load();
private:
    std::unordered_map<DeviceId, std::vector<PinId>> data;
    uint32_t version{1};
};
