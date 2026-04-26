#ifndef GLOBALS_H
#define GLOBALS_H
#include <clock.h>
#include <config.h>
#include <encryption.h>
#include <myWIFI.h>
#include <secretstore.h>
#include <setupesp.h>
#include "pin.h"
#include "internet.h"
#include "switch433.h"
#include <pcfdevice.h>
#include <tempmonitor.h>

enum PCFtype {
  Switches = 0,  // механические выключатели
  PIR,           // датчики PIR
  rezerve,        // реле
  mq2,
  end_PCFtype
};

extern const std::array<uint8_t, PCFtype::end_PCFtype> addressPCF;

void start_relay(int number_pin, bool on);

// Настройки Wi-Fi
extern MyWiFi wifi;
extern BEEP beep;
extern Internet inet;
extern CLOCK myclock;
extern TempMonitor temp_monitor;
//PCF8574
extern TwoWire I2C1;

extern std::array<PCF8574Device, PCFtype::end_PCFtype> pcfG;

enum PINSrelay {
  relay1 = 0,
  relay2,
  relay3,
  relay4,
  relay5,
  relay6,
  relay7,
  end_PINSrelay
};
extern std::array<PIN, PINSrelay::end_PINSrelay> pinsG;
extern std::array<PIR_SENSOR*, 5> pir_sensorG;
extern std::array<SwitchMechanics, 3> switch_mechanicsG;
extern Switch433 switch1;

void pcf_search(TwoWire &bus);
//void printMemoryStatus(const char* taskName = "Main");

// Для std::array объектов
template<typename T1, size_t N>
T1 *search_by_id(int id, std::array<T1, N> &arr) {
  for (auto &obj : arr) {
    if (obj.get_id() == id) return &obj;
  }
  return nullptr;
}
// Для std::array указателей
template<typename T1, size_t N>
T1* search_by_id(int id, std::array<T1*, N> &arr) {
    for (auto &obj : arr) {
        if (obj && obj->get_id() == id) return obj;
    }
    return nullptr;
}

void testPCF8574(TwoWire &I2C, uint8_t address);





#endif