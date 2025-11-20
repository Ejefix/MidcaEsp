#ifndef INTERNET_H
#define INTERNET_H
#include <WiFi.h> 
#include <Arduino.h>
#include "encryption.h"
#include <skeleton.h>
#include <config.h>
#include <clock.h>

class Internet : public Encryption, public Config,  public Skeleton {
public:
  Internet(CLOCK *myclock);
  bool connect();
  void processServerResponse();
  static const unsigned long long epochDiff;
private:

  char *read_buffer(size_t &buffer_size);
  String body();
  int get_command();
  CLOCK *myclock{ nullptr };
  
  WiFiClient client;
   
};

#endif
