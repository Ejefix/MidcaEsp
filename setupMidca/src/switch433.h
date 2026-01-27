#pragma once
#include <RCSwitch.h>
#include <vector>
#include <map>
#include "pin.h"
#include <set>
#include <ArduinoJson.h>

class Switch433 {

public:
  Switch433();

  void set_pin(size_t pin_);
  void push_PIN(size_t signal, PIN *pin);
  void remove_PIN(size_t signal, int pinNum);
  void remove_signal(size_t signal);
  void fill_json(JsonArray &arr) const;
  int begin();
  void load_from_json();
private:
  void save_to_json()const ; 
  void checkSignal();
  void processingSignal(size_t signal, bool script = false);
  void set_all(const unsigned char status);
  const unsigned char pin_off{ 0b10000100 };     // выключение
  const unsigned char pin_on{ 0b10000010 };      // включение
  const unsigned char pin_script{ 0b10001000 };  // сценарий
  size_t pin{};
  RCSwitch mySwitch;
  std::vector<std::pair<size_t,size_t>>signals;
  std::map<size_t,std::set<PIN *>>  actions_pin{};
  const String path{"/Switch433"};
};