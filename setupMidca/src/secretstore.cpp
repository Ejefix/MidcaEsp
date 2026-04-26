#include "secretstore.h"
#include "setupesp.h"
SecretStore::SecretStore() {
 secret = SETUP_ESP::readToFile(path, secretKod);
}
String SecretStore::get_secret()const {

  if (this->secret) {
    return secretKod;
  } else {
    return secretKodDefault;
  }
}
bool SecretStore::set_secret(const String &secrets) {

  if (SETUP_ESP::saveToFile(path, secrets)) {
    secretKod = secrets;
    secret = true;
    Serial.println("[LOG] обновили Secret");
  }
  else
  {
    SETUP_ESP::deleteFile(path);
    Serial.println("❌ не удалось обновить Secret");
    secret = false;
  }
  return secret;
}

