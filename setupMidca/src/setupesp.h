#ifndef SETUPESP_H
#define SETUPESP_H

#include <WiFi.h>
#include "encryption.h"
#include <array>

class SETUP_ESP : public Encryption {
public:
  explicit SETUP_ESP();  // конструктор
  
static bool saveToFile(const String &path, const String &data);
static bool readToFile(const String &path, String &data);
static void deleteFile(const String &path);
};



#endif