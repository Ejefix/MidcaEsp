
#ifndef ENCRYPTION_H
#define ENCRYPTION_H
#include <string>
#include <Arduino.h>
#include <secretstore.h>

class Encryption :  public SecretStore
{
public:
    Encryption();                       // Конструктор
    String get_hash(unsigned long long time, const String &id, const String &secret)const;
    String encrypt(const String &plaintext)const;
    String decrypt(const String &packet)const;
     String get_key()const;
private:
    std::string password{"ynC[0uPht1[G[gf!"};
    String key{};
};

#endif

