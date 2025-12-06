#include "globals.h"


std::array<PIN, 4> pins{
 // PIN{ 22 },
 // PIN{ 23 },
 // PIN{ 21 },
 // PIN{ 25 },
  PIN{ 26 },
  PIN{ 27 },
  PIN{ 32 },
  PIN{ 33 },
};
std::array<int ,2> pin_pir_sensor{16,17};

CLOCK myclock{};
Internet inet{ &myclock };
MyWiFi wifi{};
