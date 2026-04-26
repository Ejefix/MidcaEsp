#include "firmware_profile.h"
#if FW_BUILD == FW_RELAY

#include "globalsRelay.h"

const std::array<int,7> GPIOrelay{17,26,25,27,33,23,32};
const std::array<IPinDriver*,7> PinDriver{
  new RelayDriver{ GPIOrelay[0]},
  new RelayDriver{ GPIOrelay[1]},
  new RelayDriver{ GPIOrelay[2]},
  new RelayDriver{ GPIOrelay[3]},
  new RelayDriver{ GPIOrelay[4]},
  new RelayDriver{ GPIOrelay[5]},
  new RelayDriver{ GPIOrelay[6]},
};


const std::array<int,1> GPIOtemperatures{5};
const std::pair<int,int> GPIO_I2C1{21,22};
const std::pair<int,int> GPIO_Switch433{18,16};
const std::array<int,1> GPIOledPwm{12};

// 5,12,16,17,18,21,22,23,25,26,27,32,33

// свободно 4, 13, 14, 19

// 4,12,13,14,19,16,17,18,21,22,23,25,26,27,32,33
const std::array<uint8_t, PCFtype::end_PCFtype> addressPCF{
  0x20,  // механические выключатели
  0x24,  // Pir датчики
  0x22,
  0x21   
};

TwoWire I2C1{ 0 };
TempMonitor temp_monitor{GPIOtemperatures[0]};



std::array<PCF8574Device*, PCFtype::end_PCFtype> pcfG{
  new PCF8574Device{ I2C1, addressPCF[PCFtype::Switches] },  // только чтение
  new PCF8574Device{ I2C1, addressPCF[PCFtype::PIR] },       // только чтение
  new PCF8574Device{ I2C1, addressPCF[PCFtype::rezerve]},
  new PCF8574Device{ I2C1, addressPCF[PCFtype::mq2]}
};

std::array<PIN*, PINSrelay::end_PINSrelay> pinsG{
  new PIN{ PinDriver[0] }, // 12V
  new PIN{ PinDriver[1] }, // 220V
  new PIN{ PinDriver[2] },// 220V
  new PIN{ PinDriver[3]  },// 220V
  new PIN{ PinDriver[4] },// 220V
  new PIN{ PinDriver[5] },// 220V
  new PIN{ PinDriver[6]  }// 220V
};

std::array<PIR_SENSOR*, 4> pir_sensorG{
  new PIR_SENSOR{ pcfG[PCFtype::PIR], 0 },
  new PIR_SENSOR{ pcfG[PCFtype::PIR], 1 },
  new PIR_SENSOR{ pcfG[PCFtype::PIR], 2 },
  new PIR_SENSOR{ pcfG[PCFtype::PIR], 7 }
};
 
std::array<SwitchMechanics, 3> switch_mechanicsG{
  SwitchMechanics{ pcfG[PCFtype::Switches], 0 },
  SwitchMechanics{ pcfG[PCFtype::Switches], 1 },
  SwitchMechanics{ pcfG[PCFtype::Switches], 2 },
};

Switch433 switch1{};
void pcf_search(TwoWire &bus) {
  Serial.println("Scanning I2C bus...");

  for (uint8_t address = 1; address < 127; address++) {
    bus.beginTransmission(address);
    if (bus.endTransmission() == 0) {  // 0 = устройство ответило
      Serial.print("Found device at 0x");
      if (address < 16) Serial.print("0");  // для красоты
      Serial.println(address, HEX);
    }
  }

  Serial.println("Scan complete.");
}

/*
void printMemoryStatus(const char* taskName )
{
    Serial.println("=== MEMORY STATUS ===");

    // HEAP
    Serial.print("[HEAP] Free heap: ");
    Serial.println(ESP.getFreeHeap());

    Serial.print("[HEAP] Max alloc block: ");
    Serial.println(ESP.getMaxAllocHeap());

    Serial.print("[HEAP] Min free heap ever: ");
    Serial.println(ESP.getMinFreeHeap());

    // STACK (текущая задача)
    Serial.print("[STACK] ");
    Serial.print(taskName);
    Serial.print(" free stack (words): ");
    Serial.println(uxTaskGetStackHighWaterMark(NULL));

    // FLASH
    Serial.print("[FLASH] Chip size: ");
    Serial.println(ESP.getFlashChipSize());

    Serial.print("[FLASH] Speed: ");
    Serial.println(ESP.getFlashChipSpeed());

#ifdef ESP32
    // PSRAM (если есть)
    Serial.print("[PSRAM] Total: ");
    Serial.println(ESP.getPsramSize());

    Serial.print("[PSRAM] Free: ");
    Serial.println(ESP.getFreePsram());
#endif
    Serial.println("====================");
}*/



void testPCF8574(TwoWire &I2C, uint8_t address) {
    Serial.print("=== PCF8574 Test Start ");
    Serial.println(address, HEX);
    for (uint16_t val = 0; val <= 255; ++val) {
        // Запись байта
        I2C.beginTransmission(address);
        I2C.write(val & 0xFF);
        uint8_t err = I2C.endTransmission();

        if (err != 0) {
            Serial.print("[WRITE ERROR] Addr 0x");
            Serial.print(address, HEX);
            Serial.print(" | Code: ");
            Serial.println(err);
            continue; // переходим к следующему значению
        }

        delay(50); // даем линии стабилизироваться

        // Чтение байта
        I2C.requestFrom(address, (uint8_t)1);
        uint8_t read_val = 0xFF;
        if (I2C.available()) {
            read_val = I2C.read();
        }

        // Выводим только если есть расхождение
        if (read_val != (val & 0xFF)) {
            Serial.print("[PCF TEST MISMATCH] Addr 0x");
            Serial.print(address, HEX);
            Serial.print(" | Wrote: 0b");
            for (int i = 7; i >= 0; --i) Serial.print(bitRead(val, i));
            Serial.print(" | Read: 0b");
            for (int i = 7; i >= 0; --i) Serial.print(bitRead(read_val, i));
            Serial.println();
        }
    }
    Serial.println("=== PCF8574 Test End ===");
}

#endif