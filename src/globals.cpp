#include "globals.h"
#include <Wire.h>
#include <scenario_intent_system.h>

MyWiFi wifi{};
Config configG{};
CLOCK myclock{};
Internet inet{&myclock};
std::vector<Adafruit_MCP23X17 *> mcpG{};
const std::pair<int, int> wire_pair{4, 5};

DeviceRegistry *device_registry{new DeviceRegistry{}};
DeviceBinder * device_binder{new DeviceBinder {}};
ScheduledIntentStore *store{new ScheduledIntentStore{}};
Arbitrator * scheduler{new Arbitrator{*store}};
IntentExecutor *intent_executor{new IntentExecutor{}}; 
bool updateDATA{false};





const std::array<int, 15> ESP_GPIO{12, 13, 14, 19, 16, 17, 18, 21, 22, 23, 25, 26, 27, 32, 33};
std::vector<std::array<IGpioPin *, 16>> gpioSignal{};
#if FW_BUILD == FW_RELAY
const std::array<IPinDriver *, 15> PinDriver{
    new RelayDriver{ESP_GPIO[0]},
    new RelayDriver{ESP_GPIO[1]},
    new RelayDriver{ESP_GPIO[2]},
    new RelayDriver{ESP_GPIO[3]},
    new RelayDriver{ESP_GPIO[4]},
    new RelayDriver{ESP_GPIO[5]},
    new RelayDriver{ESP_GPIO[6]},
    new RelayDriver{ESP_GPIO[7]},
    new RelayDriver{ESP_GPIO[8]},
    new RelayDriver{ESP_GPIO[9]},
    new RelayDriver{ESP_GPIO[10]},
    new RelayDriver{ESP_GPIO[11]},
    new RelayDriver{ESP_GPIO[12]},
    new RelayDriver{ESP_GPIO[13]},
    new RelayDriver{ESP_GPIO[14]},
};

#elif FW_BUILD == FW_PWM
const std::array<IPinDriver *, 15> PinDriver{
    new PWMDriver{ESP_GPIO[0]},
    new PWMDriver{ESP_GPIO[1]},
    new PWMDriver{ESP_GPIO[3]},
    new PWMDriver{ESP_GPIO[4]},
    new PWMDriver{ESP_GPIO[5]},
    new PWMDriver{ESP_GPIO[6]},
    new PWMDriver{ESP_GPIO[7]},
    new PWMDriver{ESP_GPIO[8]},
    new PWMDriver{ESP_GPIO[9]},
    new PWMDriver{ESP_GPIO[10]},
    new PWMDriver{ESP_GPIO[11]},
    new PWMDriver{ESP_GPIO[12]},
    new PWMDriver{ESP_GPIO[13]},
    new PWMDriver{ESP_GPIO[14]},
    new PWMDriver{ESP_GPIO[15]},
};
#else
#error "Unknown firmware build"
#endif

std::array<PIN *, 15> pinsG{
    new PIN{PinDriver[0]},
    new PIN{PinDriver[1]},
    new PIN{PinDriver[2]},
    new PIN{PinDriver[3]},
    new PIN{PinDriver[4]},
    new PIN{PinDriver[5]},
    new PIN{PinDriver[6]},
    new PIN{PinDriver[7]},
    new PIN{PinDriver[8]},
    new PIN{PinDriver[9]},
    new PIN{PinDriver[10]},
    new PIN{PinDriver[11]},
    new PIN{PinDriver[12]},
    new PIN{PinDriver[13]},
    new PIN{PinDriver[14]}};

void setupStart()
{
  Wire.begin(wire_pair.first,wire_pair.second);
  auto addr = scanI2C();
  // инициализация MCP23X17
  for (size_t i{}; i < addr.size(); ++i)
  {
    auto mcp = new Adafruit_MCP23X17{};
    if (!mcp->begin_I2C(addr[i]))
    {
      Serial.println("MCP23017 NOT FOUND");
    }
    else
    {
      Serial.print("[INF] MCP23017 регистрация адреса ");
      Serial.println(addr[i], HEX); // вывод адреса
      mcpG.push_back(mcp);
    }
  }
  // инициализация gpioSignal
  for (size_t i{}; i < mcpG.size(); ++i)
  {
    std::array<IGpioPin *, 16> gpio{};
    for (size_t pin{}; pin < gpio.size(); ++pin) // создаем 16 пинов
    {
      gpio[pin] = new MCP23017Pin{mcpG[i], static_cast<uint8_t>(pin)};
    }
    gpioSignal.push_back(gpio);
  }
  // регистрация девайсов
  {
    device_registry->load();
    for (size_t i{}; i < mcpG.size(); ++i)
    {
      for (int z{}; z < 16; ++z)
      {
         if(device_registry->add(i, DeviceType::Sensor, z))
        {
          Serial.print("[DeviceRegistry] Зарегестрировал устройство ");
          Serial.print("MCP23X17 номер ");
          Serial.print(i);
          Serial.print(" на пин - ");
          Serial.println(z);
        }
      }
    }
  }
  for (size_t i{}; i < pinsG.size(); ++i)
  {
    //pinsG[i]->load();
    
  }
}

std::vector<uint8_t> scanI2C()
{
  Serial.println("I2C scan start"); // начало сканирования

  std::vector<uint8_t> scan{};
  for (uint8_t addr = 1; addr < 127; addr++) // перебор всех адресов
  {
    Wire.beginTransmission(addr);           // начинаем передачу
    uint8_t error = Wire.endTransmission(); // проверяем ответ

    if (error == 0) // устройство ответило
    {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX); // вывод адреса
      scan.push_back(addr);
    }
  }

  Serial.println("I2C scan done"); // завершение
  return scan;
}
