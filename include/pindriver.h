#pragma once
#include <Arduino.h>

// -------------------------
// Возможности драйвера
// -------------------------
enum DriverCaps : uint8_t
{
    None = 0,
    Brightness = 1 << 0,
    Fade = 1 << 1,
    PWM = 1 << 2,
    RGB = 1 << 3
};
inline DriverCaps operator|(DriverCaps a, DriverCaps b)
{
    return static_cast<DriverCaps>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline DriverCaps &operator|=(DriverCaps &a, DriverCaps b)
{
    a = a | b;
    return a;
}
inline bool operator&(DriverCaps a, DriverCaps b)
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}
inline bool operator!=(DriverCaps a, DriverCaps b)
{
    return static_cast<uint8_t>(a) != static_cast<uint8_t>(b);
}
// -------------------------
// БАЗОВЫЙ ИНТЕРФЕЙС
// -------------------------
class IPinDriver
{
public:
    virtual void write(bool on, uint8_t brightness) = 0; // выполнить команду
    virtual DriverCaps caps() const = 0;                 // описать возможности
    virtual ~IPinDriver() {}
};

// -------------------------
// РЕЛЕ
// -------------------------
class RelayDriver : public IPinDriver
{
public:
    explicit RelayDriver(int pin);

    void write(bool on, uint8_t) override;
    DriverCaps caps() const override;

private:
    int pin;
    bool state{false};
};

// -------------------------
// PWM (ESP32 LEDC)
// -------------------------
class PWMDriver : public IPinDriver
{
public:
    PWMDriver(int pin);

    void write(bool on, uint8_t brightness) override;
    DriverCaps caps() const override;
    uint8_t getBrightness() const;

private:
    static const int freq;
    static int nextChannel;
    const int channel{};
    int pin;
    uint8_t brightness{};
    bool state{false};
};