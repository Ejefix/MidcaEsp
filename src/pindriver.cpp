#include "PinDriver.h"
#include "pin.h"
// =========================
// RELAY DRIVER
// =========================
RelayDriver::RelayDriver(int pin) : pin(pin)
{
    pinMode(pin, OUTPUT); // пин как выход
    digitalWrite(pin, HIGH);
}

bool RelayDriver::write(bool on, uint8_t)
{

    if (state == on)
        return false;
    state = on;
    PIN::changed_flags = true;
    if (on)
    {
        digitalWrite(pin, LOW);
    }
    else
    {
        digitalWrite(pin, HIGH);
    }
    // Serial.print("[INF] записали в  ");
    // Serial.print(" пин ");
    //  Serial.print(pin);
    // Serial.print(" состояние ");
    //  Serial.println(state);
    return true;
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
    ledcAttachPin(pin, channel);            // привязка пина
}

bool PWMDriver::write(bool on, uint8_t b)
{
    if (state == on && brightness == b)
        return false;

    brightness = b; // сохраняем состояние
    state = on;
    ledcWrite(channel, on ? b : 0);
    Serial.print("[INF] записали в  канал ");
    Serial.print(channel);
    Serial.print(" пин ");
    Serial.print(pin);
    Serial.print(" яркость ");
    Serial.println(b);
    return true;
}

uint8_t PWMDriver::getBrightness() const
{
    return brightness;
}

DriverCaps PWMDriver::caps() const
{
    return DriverCaps::Brightness;
}