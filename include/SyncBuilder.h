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
    void reset();
    String begin();
    void add_buffer(const String &out);

private:
    void buildPINsSnapshot(String &out) const;
    void buildDeviceSnapshot(String &out) const;
    void buildSTORESnapshot(String &out) const;
    void buildConnectSnapshot(String &out) const;

    void buildPINsSnapshot(std::vector<PinId> ids, String &out) const;
    void buildIntentSnapshot(std::vector<ScheduledIntentID> id, String &out) const;
    void buildDeviceSnapshot(uint16_t id, String &out) const;

    static uint32_t cmd_id_count;

    String fullBody(const String &body, const String &id_ = "");
    /* void sendFullStatus();*/
    void sendUpdatePins();
    void sendUpdateDevice();
    void sendUpdateStore();
    void sendUpdateConnect();
    // удаляет из списка, если неу этих ID в магазине
    void controlversionIntent(const std::vector<ScheduledIntentID> &actual);
    std::unordered_map<ScheduledIntentID, uint32_t> versionIntent{};
    std::unordered_map<PinId, uint32_t> versionPINS{};
    std::unordered_map<uint16_t, uint32_t> versionDevice{};
    uint32_t versionStore{};
    uint32_t versionDevice_registry{};
    uint32_t versionDevice_bind{};

    Encryption enc{};
    std::deque<String> buffer;

    uint8_t counter{};

    uint32_t last_PINS{};
    uint32_t last_Device{};
    uint32_t last_Store{};
    uint32_t last_Connect{};
    int counter_buffer{};
};

class ISender
{
public:
    virtual ~ISender() = default;
    virtual bool send(const String &packet) = 0;
};
// отправка
class TCPSender : public ISender
{
    TCPSender() = delete;
    TCPSender(const TCPSender &) = delete;
    TCPSender &operator=(const TCPSender &) = delete;

public:
    explicit TCPSender(WiFiClient &client);
    bool send(const String &packet) override;

private:
    WiFiClient &client;
};
class UDPSender : public ISender
{
    UDPSender() = delete;
    UDPSender(const UDPSender &) = delete;
    UDPSender &operator=(const UDPSender &) = delete;

public:
    explicit UDPSender(WiFiUDP &client, uint16_t portUDT);

    bool update_broadcast();
    bool send(const String &packet) override;
    void set_portUDT(uint16_t portUDT);

private:
    WiFiUDP &client;
    IPAddress broadcast;
    uint16_t portUDT{};
};

// приём
class IReceiver
{
public:
    virtual ~IReceiver() = default;
    String receive();

protected:
    virtual bool hasData() = 0;
    virtual int readByte() = 0;

private:
    String read_buffer();
    const int bodyMaxSize{4500};
    String rxBuffer{};               // Накопленные TCP-данные между вызовами.
    uint16_t sizePacket{};           // Размер текущего payload.
    bool read{false};                // Заголовок текущего пакета уже обработан.
};
// приём
class TCPReceiver : public IReceiver
{
    TCPReceiver() = delete;
    TCPReceiver(const TCPReceiver &) = delete;
    TCPReceiver &operator=(const TCPReceiver &) = delete;

public:
    explicit TCPReceiver(WiFiClient &client);
    
protected:
    bool hasData() override;
    int readByte() override;

private:
    WiFiClient &client;
};
class UDPReceiver : public IReceiver
{
    UDPReceiver() = delete;
    UDPReceiver(const UDPReceiver &) = delete;
    UDPReceiver &operator=(const UDPReceiver &) = delete;

public:
    explicit UDPReceiver(WiFiUDP &udp);
    
protected:
    bool hasData() override;
    int readByte() override;

private:
    WiFiUDP &client;
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
    int communication_socet(String &packet);

private:
    int communication_socet();
    String read_buffer();
    bool searhID();
    bool isCommandProcessed(const String &id);
    void parseIntent(const String &jsonStr);
    WiFiClient &client;
    Encryption enc{};
    static std::deque<String> history;
    const unsigned long timeout{100}; //  таймаут
    const int bodyMaxSize{4500};
};

/* Главный контроллер TCP */
class ClientTCP
{
public:
    ClientTCP() = delete;
    explicit ClientTCP(WiFiClient &&client, CLOCK &myclock);
    ~ClientTCP();

    bool begin(const String &packet);
    bool isConnected();
    void set_isAuth(bool isAuth);
    void set_adr(String adr);
    void set_port(uint16_t port);
    void set_UDT_data(String &data);
    ClientTCP(const ClientTCP &) = delete;
    ClientTCP &operator=(const ClientTCP &) = delete;

    ClientTCP(ClientTCP &&) noexcept = default;
    ClientTCP &operator=(ClientTCP &&) noexcept = default;

private:
    bool isAuth{true};
    WiFiClient client; // внешняя ссылка, не копируем
    Authorization auth;
    TCPSender session;
    TCPReceiver receiver;
    uint32_t time_reset{};
    uint32_t time_full_update{};
};

class NetworkManager
{
    NetworkManager(const NetworkManager &) = delete;
    NetworkManager &operator=(const NetworkManager &) = delete;

public:
    NetworkManager(CLOCK &myclock);
    bool update_setup();
    bool begin();
    void reset();

private:
    void parseIntent(String &packet);
    ClientTCP serwer;
    UDPSender udpSender;
    UDPReceiver udpReceiver;
    SyncBuilder builder{};
    WiFiUDP Udp{};
    WiFiUDP UdpReceiver{};
};