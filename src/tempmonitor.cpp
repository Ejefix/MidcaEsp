#include <tempmonitor.h>

TempMonitor::TempMonitor(unsigned int pin)
  : oneWire(pin), sensors(&oneWire) {
    pinMode(pin, INPUT);
}
TempMonitor::~TempMonitor() {
  delete tempID;
}

void TempMonitor::setup() {
  
  sensors.begin();
  DeviceAddress addr;
  int count = sensors.getDeviceCount();
  Serial.print("Найдено датчиков: ");
  Serial.println(count);
  if (tempID == nullptr) {
    tempID = new std::vector<std::pair<float, uint64_t>>();
  }
  for (int i = 0; i < count; ++i) {
    if (sensors.getAddress(addr, i)) {
      // Вывод в HEX
      Serial.print("Датчик #");
      Serial.print(i);
      Serial.print(" ID: ");
      // Преобразуем в число uint64_t
      uint64_t uid = 0;
      for (uint8_t b = 0; b < 8; b++) {
        uid = (uid << 8) | addr[b];
      }
      Serial.println(uid);
      tempID->push_back({ 0, uid });
      set = true;
    } else {
      Serial.print("Датчик #");
      Serial.print(i);
      Serial.println(" не найден!");
    }
  }
}
void TempMonitor::begin() {
  if (!set) {
    setup();
  }
  unsigned long now = millis();
  if (now - lastRead > readInterval) {
    lastRead = now;
    sensors.requestTemperatures();
    for (size_t i{}; i < tempID->size(); ++i) {
      tempID->at(i).first = sensors.getTempCByIndex(i);
    }
  }
}

void TempMonitor::fill_json(JsonArray &arr) const {
  for (auto it = tempID->begin(); it != tempID->end(); ++it) {
    JsonObject obj = arr.add<JsonObject>();
    obj["i"] = it->second;
    obj["t"] = it->first;
  }
}
