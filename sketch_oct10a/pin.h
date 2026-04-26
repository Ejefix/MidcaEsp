#ifndef PIN_H
#define PIN_H
#include <vector>
#include <skeleton.h>
#include "commandexecutor.h"
#include <ArduinoJson.h>
#include <Wire.h>


using RelayFunc = void (*)(int, bool);
class PCF8574Device;
class BEEP;

class PIR_SENSOR {
public:
  PIR_SENSOR() = delete;
  virtual ~PIR_SENSOR() = default;

  explicit PIR_SENSOR(unsigned int pinNumber_);
  explicit PIR_SENSOR(PCF8574Device &pcf_, uint8_t number_bit_);

  void push_script_time(std::pair<unsigned long long, unsigned long long> &time);
  void set_interval(const unsigned long &time_off);
  virtual void set_status(bool act);
  virtual bool get_activ();
  unsigned int get_id() const;
  void fill_json(JsonArray &arr) const;
  void load();
  static bool changed_flags;
protected:
  String arr_name{ "pir" };
private:
  void save() const;
  bool status{ true };
  uint64_t time_activ{};
  uint64_t time_on{};
  unsigned long time_off_{ 500 };
  const unsigned int pinNumber;
  PCF8574Device *pcf;
  const uint8_t number_bit;
  const unsigned int id;
  static unsigned int id_s;
  String path;
  unsigned long changed_flags_time{};
  std::pair<uint64_t, uint64_t> script_time{{8 * 60 * 60 * 1000}, {17 * 60 * 60 * 1000}};
};

class MQ_SENSOR : public PIR_SENSOR {
public:
  MQ_SENSOR() = delete;

  explicit MQ_SENSOR(PCF8574Device &pcf_, uint8_t number_bit_, BEEP &beep_);
  bool get_activ() override;
  void set_status(bool act) override;
private:
  BEEP *beep;
};



class PIN : public Skeleton {
public:
  PIN() = delete;
  PIN(const PIN &) = delete;
  PIN &operator=(const PIN &) = delete;

  explicit PIN(unsigned int pin, RelayFunc func);
  explicit PIN(PCF8574Device &pcf_, uint8_t number_bit_);

  void load();
  void push_script_time(std::pair<unsigned long long, unsigned long long> &time);

  bool clear_script_time(unsigned long long &time);
  void set_status_user(unsigned char pin_info_, bool save_ = true);
  void begin();

  void save() const;
  unsigned int get_pinNumber() const;
  unsigned int get_id() const;

  void add__pir_sensor_id(int id, bool save_ = true);
  void remove__pir_sensor_id(int id, bool save_ = true);
  void add_pir_sensor(PIR_SENSOR *pir, bool save_ = true);
  void remove_pir_sensor(PIR_SENSOR *pir, bool save_ = true);

  bool get_user_on() const;
  void set_user_on(bool status);
  static bool changed_flags;
  void fill_json(JsonArray &arr) const;

private:
  static unsigned int id_pin;
  friend CommandExecutor;
  enum PinFlags : unsigned char {
    FLAG_STATUS_PIN = 1 << 0,  // состояние
    FLAG_USER_ON = 1 << 1,     // вручную включён
    FLAG_USER_OFF = 1 << 2,    // вручную выключен
    FLAG_SCRIPT = 1 << 3,      // сценарий активен
    FLAG_PIR_SENSOR = 1 << 4,  // движение (PIR)
  };
  void set_pir_sensor();
  void set_status_pin(bool status);
  bool isPinActivation(const unsigned long long &time);

  const unsigned int pinNumber;
  std::vector<std::pair<uint64_t, uint64_t>> script_time{};

  //  |=      <— ставим бит в 1
  //  &= ~    <— сбрасываем бит в 0
  //  &       <— проверяем состояние
  unsigned char pin_info{ 0b10001000 };
  unsigned char pin_type{ 'A' };
  unsigned long time_delay{};
  const unsigned long time_interval{ 1000 };
  std::vector<PIR_SENSOR *> pirs{};
  RelayFunc startRelay;
  PCF8574Device *pcf;
  const uint8_t number_bit;
  const unsigned int id;
};

class BEEP {
public:
  explicit BEEP(unsigned int pin, RelayFunc func, unsigned int frequency = 0);

  BEEP() = delete;
  void update();                        // сердце, переключает состояние пищалки по времени и завершает сигнал после нужного числа шагов.
  void gasBeep();                       // включает пищалку для газа
  void startBeep(unsigned long count);  // обычные писки
  bool get_activ();                     // возвращает, идёт ли сигнал прямо сейчас
  void set_stop(bool stop_);            // принудительно выключить и разрешить активацию
private:
  unsigned int pinNumber{ 0 };
  RelayFunc startRelay;
  unsigned int frequency{ 0 };
  bool active{ false };
  bool state{ true };
  bool stop{ false };
  int step{ 0 };
  int totalSteps{ 0 };
  unsigned long time_beep_start{ 0 };
  unsigned long interval{ 0 };
  void start(int times);
};

class FlowMeter {
public:
  explicit FlowMeter(uint8_t pin, PIN *pin_);
  FlowMeter() = delete;
  void setup();
  void begin();
  volatile uint32_t pulses = 0;
  static bool on_off;
private:
  bool openWater();
  bool closeWater();
  uint8_t _pin;  // GPIO
  //volatile uint32_t pulses = 0;
  static void IRAM_ATTR isr();
  static FlowMeter *instance;
  unsigned long startTime{ 0 };
  bool leak;
  const unsigned long interval = 3000;  // интервал проверки каждые 15 секунд
  uint8_t leakCount{ 0 };
  uint8_t noLeakCount{ 0 };
  unsigned long closeTime{ 0 };
  unsigned long openTime{ 0 };
  bool open{ true };
  bool close{ false };

  PIN *pinWater{ nullptr };
};




#endif  // PIN_H
