#pragma once
#include <skeleton.h>
#include <WiFiClient.h>
#include <memory>
#include "encryption.h"

using PinId = uint16_t;

class SyncBuilder : private Skeleton
{
public:
    void buildPINsSnapshot(String &out) const;
    void buildDeviceSnapshot(String &out) const;
    void buildIntentSnapshot(String &out) const;
    void buildConnectSnapshot(String &out) const;

    void buildPINsSnapshot(PinId id, String &out) const;
    void buildIntentSnapshot(ScheduledIntentID id, String &out) const;
    void buildDeviceSnapshot(uint16_t id, String &out) const;
private:
};

// отправка
class ClientStreamSession
{
public:
    ClientStreamSession() = delete;
    explicit ClientStreamSession(WiFiClient &client_);
    void begin();
    void reset();

private:
    static uint32_t cmd_id_count;
    void to_send(WiFiClient &client_, const String &body, const String &id_ = "");
   
   /* void sendFullStatus();*/
    void sendUpdatePins();
    void sendUpdateDevice();
    void sendUpdateStore();
    void sendUpdateConnect();
    std::unordered_map<ScheduledIntentID, uint32_t> versionIntent{};
    std::unordered_map<PinId, uint32_t> versionPINS{};
    std::unordered_map<uint16_t, uint32_t> versionDevice{};
    uint32_t versionStore{};
    uint32_t versionDevice_registry{};
    uint32_t versionDevice_bind{};
    WiFiClient &client;
    SyncBuilder builder{};
    Encryption enc{};
};

// приём
class ClientStreamReceiver
{
public:
    ClientStreamReceiver() = delete;
    explicit ClientStreamReceiver(WiFiClient &client_);
    void begin();

private:
    int communication_socet();
    String read_buffer();
    WiFiClient &client;
    Encryption enc{};
};

/* Главный контроллер TCP */
class ClientTCP
{
public:
    ClientTCP() = delete;
    explicit ClientTCP(WiFiClient &client_);
    ~ClientTCP() = default;

    void begin();

private:
    WiFiClient &client; // внешняя ссылка, не копируем
    ClientStreamSession session;
    ClientStreamReceiver receiver;
};
