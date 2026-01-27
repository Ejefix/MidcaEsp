#ifndef SECRETSTORE_H
#define SECRETSTORE_H

#include <Arduino.h>  // базовые типы Arduino
#include <SPIFFS.h>   // файловая система SPIFFS

class SecretStore {
public:
  SecretStore();  // конструктор с путём файла
  String get_secret()const;
protected:
  bool set_secret(const String &secrets);
 
private:
  String secretKod{};
  String secretKodDefault{"?6G:N0$ua;0n[?hu"};
  const String path{ "/secret_part.bin" };  // путь к файлу в SPIFFS
  bool secret{ false };
};

#endif
