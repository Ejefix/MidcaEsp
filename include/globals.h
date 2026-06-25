#ifndef GLOBALS_H
#define GLOBALS_H
#include "pindriver.h"
#include "pin.h"
#include "myWIFI.h"
#include "internet.h"
#include <Adafruit_MCP23X17.h> 
#include "gpio_pin.h"
#include "arbitrator.h"
#include "intentExecutor.h"

#define FW_RELAY 1
#define FW_PWM   2

#define FW_BUILD FW_PWM


// Настройки Wi-Fi
extern MyWiFi wifi;
extern Config configG;
extern Internet inet;
extern CLOCK myclock;
extern std::vector<Adafruit_MCP23X17*> mcpG;
extern const std::pair<int,int>  wire_pair;
extern ScheduledIntentStore *store;
extern Arbitrator * arbitrator;
extern DeviceRegistry* device_registry;
extern DeviceBinder * device_binder;
extern IntentExecutor *intent_executor;

extern const std::array<int,15> ESP_GPIO;
extern std::vector<std::array<IGpioPin*,16>> gpioSignal;
extern const std::array<IPinDriver*,15> PinDriver;
extern std::array<PIN*, 15> pinsG;
extern const std::array<IGpioPin*,16> pinsMcpG;

void setupStart();
std::vector<uint8_t> scanI2C();
extern bool updateDATA;
#endif