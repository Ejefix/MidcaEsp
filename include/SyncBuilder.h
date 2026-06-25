#pragma once
#include <skeleton.h>
#include <WiFiClient.h>
#include <memory>
#include "encryption.h"
#include <deque>
#include <unordered_map>
#include "authorization.h"

using PinId = uint16_t;

class SyncBuilder 
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
    void ping_pong();
    void reset();
    ClientStreamSession(const ClientStreamSession &) = delete;
    ClientStreamSession &operator=(const ClientStreamSession &) = delete;

private:
    static uint32_t cmd_id_count;
    void to_send(WiFiClient &client_, const String &body);
    String fullBody(const String &body, const String &id_ = "");
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
    std::deque<String> buffer;
    uint32_t time_send{};
    uint8_t counter{};
};

// приём
class ClientStreamReceiver
{
public:
    ClientStreamReceiver() = delete;
    explicit ClientStreamReceiver(WiFiClient &client_);
    int begin();
    ClientStreamReceiver(const ClientStreamReceiver &) = delete;
    ClientStreamReceiver &operator=(const ClientStreamReceiver &) = delete;

private:
    int communication_socet();
    String read_buffer();
    bool isCommandProcessed(const String &id);
    void parseIntent(const String &jsonStr);
    WiFiClient &client;
    Encryption enc{};
    static std::deque<String> history;
};

/* Главный контроллер TCP */
class ClientTCP
{
public:
    ClientTCP() = delete;
    explicit ClientTCP(WiFiClient &&client, CLOCK &myclock, bool isAuth);
    ~ClientTCP();

    bool begin();
    bool isConnected();
    void set_isAuth(bool isAuth);
    void set_adr(String adr);
    void set_port(uint16_t port);

    ClientTCP(const ClientTCP &) = delete;
    ClientTCP &operator=(const ClientTCP &) = delete;

    ClientTCP(ClientTCP &&) noexcept = default;
    ClientTCP &operator=(ClientTCP &&) noexcept = default;

private:
    bool isAuth{false};
    WiFiClient client; // внешняя ссылка, не копируем
    Authorization auth;
    ClientStreamSession session;
    ClientStreamReceiver receiver;
    uint32_t time_reset{};
};
