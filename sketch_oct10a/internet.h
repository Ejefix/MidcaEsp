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
  explicit Internet(CLOCK *myclock);
  bool connect();
  void processServerResponse();
  static const unsigned long long epochDiff;
private:
  void to_send(const String &body,const String &id_ = "");

  // старый не используется
  char *read_buffer(size_t &buffer_size);
  
  String read_buffer();

  String body();
  int get_command();
  CLOCK *myclock{ nullptr };
  
  WiFiClient client;
  unsigned long go_fullstatus{};
  const unsigned long interval = 2 * 60 * 1000; 

};

#endif
