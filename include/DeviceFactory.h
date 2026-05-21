#pragma once
#include "IInputDevice.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <ArduinoJson.h>
using DeviceId = uint32_t;


struct Event
{
    DeviceId id; // кто отправил
    DeviceType type;
    InputEvent event;
};


class DeviceFactory
{
public: 
    static IInputDevice *create(uint8_t mcp_id, DeviceType type, uint8_t pin_, uint8_t id);
};

class DeviceRegistry
{
public:
    DeviceRegistry();
    // возвращает ID девайса , если 0 = error
    bool add(uint8_t mcp_id, DeviceType type, uint8_t pin);
    bool exists(uint16_t id) const;
    std::vector<std::pair<uint16_t, DeviceType>> get_ids() const;
    bool set_type(DeviceType type, uint16_t id);
    IInputDevice *get(uint16_t id);
    void fill_json(JsonArray &arr) const;
    void save() const;
    void load();
    static bool changed_flags;
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
};

class PIN;
class DeviceBinder
{

public:
    DeviceBinder();
    // если не смогли создать вернет 0
    void connect(DeviceId device, PIN *obj);

    void disconnect(DeviceId device, PIN *obj);
    void disconnect(DeviceId device);
    void disconnect(PIN *obj);
    void disconnect();
    void begin() const;

private:
    
    std::unordered_map<DeviceId, std::vector<PIN *>> data;
};
