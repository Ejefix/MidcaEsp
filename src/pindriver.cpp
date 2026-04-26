#include "PinDriver.h"

// =========================
// RELAY DRIVER
// =========================
RelayDriver::RelayDriver(int pin) : pin(pin)
{
    pinMode(pin, OUTPUT); // пин как выход
    digitalWrite(pin, HIGH);
}

void RelayDriver::write(bool on, uint8_t)
{

    if (state == on)
        return;
    state = on;

    if (on)
    {
        digitalWrite(pin, LOW);
    }
    else
    {
        digitalWrite(pin, HIGH);
    }
}

DriverCaps RelayDriver::caps() const
{
    return {};
}

// =========================
// PWM DRIVER
// =========================

const int PWMDriver::freq{15000};
int PWMDriver::nextChannel{};

PWMDriver::PWMDriver(int pin)
    : pin(pin), channel(nextChannel), brightness(0)
{
    ++nextChannel;
    ledcSetup(channel, PWMDriver::freq, 8); // 8-bit PWM
    ledcAttachPin(pin, channel); // привязка пина
}

void PWMDriver::write(bool on, uint8_t b)
{
    if (state == on && brightness == b)
        return; // ничего не изменилось

    brightness = b;                 // сохраняем состояние
    state = on;
    ledcWrite(channel, on ? b : 0);
    Serial.print("[INF] записали в  канал ");
    Serial.print(channel);
    Serial.print(" пин ");
    Serial.print(pin);
    Serial.print(" яркость ");
    Serial.println(b);
}

uint8_t PWMDriver::getBrightness() const
{
    return brightness;
}

DriverCaps PWMDriver::caps() const
{
    return DriverCaps::Brightness | DriverCaps::PWM;
}