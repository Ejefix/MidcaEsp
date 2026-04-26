#pragma once
#include <array>
#include "pindriver.h"
#include "pin.h"



extern const std::array<int,15> GPIOledPwm;
extern const std::array<IPinDriver*,15> PinDriver;
extern std::array<PIN*, 15> pinsG;