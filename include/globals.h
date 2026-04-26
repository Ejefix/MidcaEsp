#ifndef GLOBALS_H
#define GLOBALS_H

#include "firmware_profile.h"  // здесь выбор прошивки

#if FW_BUILD == FW_RELAY
  #include "globalsRelay.h"
#elif FW_BUILD == FW_PWM
  #include "globalsPWM.h"
#else
  #error "Unknown firmware build"
#endif
#include "myWIFI.h"
#include "internet.h"

// Настройки Wi-Fi
extern MyWiFi wifi;
extern Internet inet;
extern CLOCK myclock;


#endif