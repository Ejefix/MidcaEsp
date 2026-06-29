#pragma once
#include <Arduino.h>
#include "skeleton.h"
#include <WiFi.h>
#include "clock.h"

class Authorization 
{
public:
    explicit Authorization(WiFiClient &client, CLOCK &myclock);
    bool authorize();
    void reset();
    void set_adr(String adr);
    void set_port(uint16_t port);
    Authorization(const Authorization &) = delete;
    Authorization &operator=(const Authorization &) = delete;

private:
    String bodyAuthorization();
    String adr{"api.midca.ru"};
    uint16_t port{64530};
    char *read_buffer(size_t &buffer_size);
    bool authorization();
    uint32_t authRetryTimer{};
    const uint32_t authPause{1000*20};
    WiFiClient &client;
    CLOCK &myclock;
    bool connect{false};
    bool requestAuthorization{false};
};