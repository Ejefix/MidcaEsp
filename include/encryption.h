
#ifndef ENCRYPTION_H
#define ENCRYPTION_H
#include <string>
#include <Arduino.h>
#include <secretstore.h>

class Encryption :  public SecretStore
{
public:
    Encryption();                       // Конструктор
    String get_hash(unsigned long long time , const String &secret, const String &id)const;
    String encrypt(const String &plaintext)const;
    String decrypt(const String &packet)const;
private:
    std::string password{"ynC[0uPht1[G[gf!"};
};

#endif

