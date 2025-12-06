#ifndef GLOBALS_H
#define GLOBALS_H
#include "pin.h"
#include "internet.h"
#include <clock.h>
#include <myWIFI.h>

extern std::array<PIN,4> pins;
extern std::array<int ,2> pin_pir_sensor;
#define PCF 0x20         // адрес PCF8574 (обычно 0x20)

extern CLOCK myclock;
extern Internet inet;
// Настройки Wi-Fi
extern MyWiFi wifi;


#endif