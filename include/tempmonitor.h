#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <vector>

class TempMonitor {

public:
  TempMonitor(unsigned int pin);
  ~TempMonitor();
  void begin();
  void fill_json(JsonArray &arr) const;
private:
  void setup();
  bool set{ false };
  std::vector<std::pair<float, uint64_t>> *tempID{ nullptr };
 
  OneWire oneWire;
  DallasTemperature sensors;  
  unsigned long lastRead{};
  const unsigned long readInterval = 60 * 1000;
};


