#ifndef PIN_H
#define PIN_H
#include <vector>
#include <skeleton.h>
#include "commandexecutor.h"

class PIN : public Skeleton {
public:
  PIN(unsigned int pin);
  void push_script_time(std::pair<unsigned long long, unsigned long long> &time);
  bool clear_script_time(unsigned long long &time);

  void set_status_pin(bool status);

  void set_status_user(unsigned char pin_info_);

  bool isPinActivation(const unsigned long long &time);
  String get_full_status_pin() const;
  String get_status_pin() const;

  void save() const;
  unsigned int get_pinNumber() const;

  // мы можем сказать только, что датчик активен. p.s. pin сам решит нужно ли выключать
  void set_pir_sensor(bool on);

  void set_pir_sensor_interval(const unsigned long &time_off);
  void set_USER_ON(bool status);
  static bool changed_flags;
private:
  friend CommandExecutor;
  enum PinFlags : unsigned char {
    FLAG_STATUS_PIN     = 1 << 0,   // состояние 
    FLAG_USER_ON        = 1 << 1,   // вручную включён
    FLAG_USER_OFF       = 1 << 2,   // вручную выключен
    FLAG_SCRIPT         = 1 << 3,   // сценарий активен
    FLAG_PIR_SENSOR     = 1 << 4,   // движение (PIR)
    FLAG_SWITCH_STATE   = 1 << 5    // механический выключатель
};

  static bool changed_script;

  void load();

  unsigned int pinNumber{ 0 };

  std::vector<std::pair<uint64_t, uint64_t>> script_time{};


  unsigned char pin_type{ 'A' };
  //  |=      <— ставим бит в 1
  //  &= ~    <— сбрасываем бит в 0
  //  &       <— проверяем состояние
  unsigned char pin_info{ 0b10001000 };

  const uint64_t one_day = 24ULL * 60ULL * 60ULL * 1000ULL;

  uint64_t pir_sensor_time_on{};
  unsigned long time_off_{10000};
  unsigned long time_delay{};
  const unsigned long time_interval{10000};
  uint64_t time_pir_activ{};
};

#endif  // PIN_H
