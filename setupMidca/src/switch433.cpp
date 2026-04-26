#include "switch433.h"
#include "globals.h"
#include "pin.h"

Switch433::Switch433() {
  signals.reserve(30);
}

void Switch433::set_pin(size_t pin_) {
  pin = pin_;
  mySwitch.enableReceive(pin);
}


int Switch433::begin() {
  if (pin == 0) return -1;
  unsigned long now = millis();
  if (signals.size() == 1) {
    signals[0].second = now;
  }
  if (signals.empty()) {
    signals.emplace_back(0, now);
  }
  if (mySwitch.available()) {  // если сигнал принят
    auto signal = mySwitch.getReceivedValue();
    mySwitch.resetAvailable();  // очистить флаг
    auto it_signal = actions_pin.find(signal);
    if (it_signal != actions_pin.end()) {
      signals.emplace_back(signal, now);
      Serial.print("[LOG] Signal ");
      Serial.print(signal);
      Serial.println(" добален");
    } else {
      if (2111654 == signal) {
        FlowMeter::on_off = !FlowMeter::on_off;
        Serial.println("[INF] блокировка воды изменина");
      }
      
    }
  }
  if (signals.size() >= 2) {
    auto &last = signals.back();
    auto timestamp = last.second;
    if (now - timestamp > 200) {
      signals.emplace_back(0, now);
    }
  }
  checkSignal();
  if (signals.size() > 30) {
    signals.clear();
  }
  return 0;
}




void Switch433::push_PIN(size_t signal, PIN *pin) {
  actions_pin[signal].insert(pin);
  save_to_json();
}
void Switch433::save_to_json() const {
  DynamicJsonDocument doc(1024);  // JSON в памяти

  for (auto &pair : actions_pin) {                              // перебор map
    JsonArray arr = doc.createNestedArray(String(pair.first));  // ключ = signal
    for (PIN *pin : pair.second) {                              // перебор set<PIN*>
      arr.add(pin->get_pinNumber());                            // сохраняем номер пина
    }
  }

  File file = SPIFFS.open(path, FILE_WRITE);  // открываем файл
  if (file) {
    serializeJson(doc, file);  // пишем в файл
    file.close();              // закрываем файл
  }
  Serial.println("[LOG] JSON сохранён:");  // лог
  serializeJsonPretty(doc, Serial);        // выводим JSON в Serial
  Serial.println();                        // перенос строки
}
void Switch433::load_from_json() {
  File file = SPIFFS.open(path, FILE_READ);  // открыть файл
  if (!file) {
    Serial.println("[ERR] JSON файл не найден");
    return;
  }
  Serial.println("[DBG] Содержимое файла:");

  DynamicJsonDocument doc(1024);  // буфер JSON
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if (err) {
    Serial.print("[ERR] JSON ошибка: ");
    Serial.println(err.c_str());
    Serial.print("file ");
    Serial.println(path);
    return;
  }

  Serial.println("[LOG] JSON загружен из файла:");
  Serial.println("-----------------------------");

  for (JsonPair kv : doc.as<JsonObject>()) {
    size_t signal = atoi(kv.key().c_str());  // конвертация ключа

    Serial.print("[SIGNAL] ");
    Serial.println(signal);

    JsonArray arr = kv.value().as<JsonArray>();

    for (JsonVariant v : arr) {
      int pinNum = v.as<int>();
      Serial.print("   └─ pin: ");
      Serial.println(pinNum);
      for (size_t i{}; i < pins.size(); ++i) {
        auto pinNum_ = pins[i].get_pinNumber();
        if (pinNum_ == pinNum) {
          actions_pin[signal].insert(&pins[i]);
          break;
        }
      }
    }
    Serial.println();
  }

  Serial.println("[LOG] Загрузка завершена");
  Serial.println("-----------------------------");
}
void Switch433::remove_PIN(size_t signal, int pinNum) {
  auto it_signal = actions_pin.find(signal);  // ищем ключ
  if (it_signal == actions_pin.end()) {
    Serial.print("[LOG] Signal ");
    Serial.print(signal);
    Serial.println(" не найден");
    return;
  }

  auto &pin_set = it_signal->second;  // set<PIN*>
  bool found = false;

  for (auto it = pin_set.begin(); it != pin_set.end(); ++it) {
    if ((*it)->get_pinNumber() == pinNum) {
      it = pin_set.erase(it);  // удаляем
      found = true;
      break;
    }
  }

  if (found) {
    Serial.print("[LOG] PIN ");
    Serial.print(pinNum);
    Serial.print(" удалён из signal ");
    Serial.println(signal);
  } else {
    Serial.print("[LOG] PIN ");
    Serial.print(pinNum);
    Serial.print(" не найден в signal ");
    Serial.println(signal);
  }

  save_to_json();  // сохраняем изменения
}
void Switch433::remove_signal(size_t signal) {
  auto it_signal = actions_pin.find(signal);  // ищем ключ
  if (it_signal == actions_pin.end()) {
    Serial.print("[LOG] Signal ");
    Serial.print(signal);
    Serial.println(" не найден");
    return;
  }
  actions_pin.erase(it_signal);
  Serial.print("[LOG] Signal ");
  Serial.print(signal);
  Serial.println(" удалён");
  for (const auto &it : actions_pin) {
    Serial.print("[LOG] остались Signal ");
    Serial.println(it.first);
  }
  save_to_json();
}


void Switch433::checkSignal() {
  // Проверка минимального количества сигналов и последнего нуля
  if (signals.size() < 3 || signals.back().first != 0) return;
  auto size = signals.size();
  std::set<size_t> sig{};
  // собираем сигналы без повторов
  for (size_t i{}; i < size; ++i) {
    if (signals[i].first != 0) {
      sig.insert(signals[i].first);
    }
  }
  for (auto real_signal : sig) {
    Serial.print("[LOG] Signal ");
    Serial.print(real_signal);
    Serial.println(" обрабатывется");
  }
  if (sig.size() == 1) {
    auto real_signal = *sig.begin();  // единственный реальный сигнал
    if (size <= 5) {
      processingSignal(real_signal);
    }
    if (size > 5 && size < 9) {
      set_all(pin_off);
    }
    if (size > 8) {
      processingSignal(real_signal, true);
    }
    signals.clear();  // сигнал обработан, удаляем
  } else {
    for (auto real_signal : sig) {
      Serial.print("[LOG] Signal ");
      Serial.print(real_signal);
      Serial.println(" обрабатывется");
      processingSignal(real_signal);
    }
    signals.clear();  // сигнал обработан, удаляем
  }

  return;
}
void Switch433::processingSignal(size_t signal, bool script) {
  auto it_signal = actions_pin.find(signal);
  if (it_signal != actions_pin.end()) {

    auto &pin_set = it_signal->second;
    if (script) {
      for (auto it = pin_set.begin(); it != pin_set.end(); ++it) {
        (*it)->set_status_user(pin_script);
      }
    } else {
      for (auto it = pin_set.begin(); it != pin_set.end(); ++it) {
        (*it)->set_switch_state_int(1);
      }
    }
    Serial.print("[LOG] Signal ");
    Serial.print(signal);
    Serial.println(" обработан");
  }
}
void Switch433::set_all(const unsigned char status) {

  for (auto it_map = actions_pin.begin(); it_map != actions_pin.end(); ++it_map) {
    auto &pin_set = it_map->second;  // set<PIN*>
    for (auto it_set = pin_set.begin(); it_set != pin_set.end(); ++it_set) {
      (*it_set)->set_status_user(status);
    }
  }
}
void Switch433::fill_json(JsonArray &arr) const {
  for (const auto &p : actions_pin) {
    JsonObject obj = arr.createNestedObject();
    obj["signal"] = p.first;

    JsonArray pinsArr = obj.createNestedArray("pin");
    for (PIN *pin : p.second) {
      pinsArr.add(pin->get_pinNumber());
    }
  }
}
