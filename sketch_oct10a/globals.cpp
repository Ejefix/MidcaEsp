#include "globals.h"


const std::array<uint8_t, PCFtype::end_PCFtype> addressPCF{
  0x20,  // механические выключатели
  0x24,  // Pir датчики
  0x22,
  0x21   
};
BEEP beep{16,start_relay,2000};
CLOCK myclock{};
Internet inet{ &myclock };
MyWiFi wifi{};
TwoWire I2C1{ 0 };
TempMonitor temp_monitor{5};

void start_relay(int number_pin, bool on) {
  if (on) {
    digitalWrite(number_pin, LOW);
  } else {
    digitalWrite(number_pin, HIGH);
  }
}

std::array<PCF8574Device, PCFtype::end_PCFtype> pcfG{
  PCF8574Device{ I2C1, addressPCF[PCFtype::Switches] },  // только чтение
  PCF8574Device{ I2C1, addressPCF[PCFtype::PIR] },       // только чтение
  PCF8574Device{ I2C1, addressPCF[PCFtype::rezerve]},
  PCF8574Device{ I2C1, addressPCF[PCFtype::mq2]}
};

std::array<PIN, PINSrelay::end_PINSrelay> pinsG{
  PIN{ 17, start_relay }, // 12V
  PIN{ 26, start_relay }, // 220V
  PIN{ 25, start_relay },// 220V
  PIN{ 27, start_relay },// 220V
  PIN{ 33, start_relay },// 220V
  PIN{ 23, start_relay },// 220V
  PIN{ 32, start_relay }// 220V
};

std::array<PIR_SENSOR*, 5> pir_sensorG{
  new PIR_SENSOR{ pcfG[PCFtype::PIR], 0 },
  new PIR_SENSOR{ pcfG[PCFtype::PIR], 1 },
  new PIR_SENSOR{ pcfG[PCFtype::PIR], 2 },
  new PIR_SENSOR{ pcfG[PCFtype::PIR], 7 },
  new MQ_SENSOR(pcfG[PCFtype::PIR],6, beep),
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

