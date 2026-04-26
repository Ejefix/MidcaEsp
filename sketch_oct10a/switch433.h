#pragma once
#include <RCSwitch.h>
#include <vector>
#include <map>
#include <set>
#include <ArduinoJson.h>

class PIN;
class PCF8574Device;

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
  void save_to_json() const;
  void checkSignal();
  void processingSignal(size_t signal, bool script = false);
  void set_all(const unsigned char status);
  const unsigned char pin_off{ 0b10000100 };     // выключение
  const unsigned char pin_on{ 0b10000010 };      // включение
  const unsigned char pin_script{ 0b10001000 };  // сценарий
  size_t pin{};
  RCSwitch mySwitch;
  std::vector<std::pair<size_t, size_t>> signals;
  std::map<size_t, std::set<PIN *>> actions_pin{};
  const String path{ "/Switch433" };
};

class SwitchMechanics {
public:
  SwitchMechanics() = delete;
  explicit SwitchMechanics(PCF8574Device &pcf_, uint8_t number_bit_);

  void begin();

  unsigned int get_id() const;

  void push_PIN_id(unsigned int id, bool save_ = true);
  void remove_PIN_id(unsigned int id, bool save_ = true);

  void push_PIN(PIN *pin, bool save_ = true);
  void remove_PIN(PIN *pin, bool save_ = true);


  void fill_json(JsonArray &arr) const;
  void load();
  
private:
  bool get_activ();
  void save() const;

  PCF8574Device *pcf;
  const uint8_t number_bit;

  static unsigned int id_switch;
  unsigned int id{};
  bool status{ false };
  std::vector<PIN *> pins;
  String path{false};
};
