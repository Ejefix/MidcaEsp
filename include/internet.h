#ifndef INTERNET_H
#define INTERNET_H
#include <WiFi.h>
#include <Arduino.h>
#include "encryption.h"
#include <skeleton.h>
#include <config.h>
#include <clock.h>
#include <WebServer.h>

class Internet : public Encryption, public Config, public Skeleton
{
public:
  explicit Internet(CLOCK *myclock);
  bool connect();
  void startTCPSerwer();
  void processServerResponse();
  static const unsigned long long epochDiff;

private:
  int communication_socet(WiFiClient &client_);
  void to_send(WiFiClient &client_, const String &body, const String &id_ = "");
  int process_cmd(String &decrypted);
  // только авторизация
  char *read_buffer(size_t &buffer_size);
  void ping_pong();
  void full_status(WiFiClient &client_);
  String read_buffer(WiFiClient &client_);
  void communication_udp();
  String body();
  bool authorization();
  CLOCK *myclock{nullptr};

  // Порт TCP сервера
  const uint16_t TCP_PORT = 9000;

  // TCP сервер
  WiFiServer *tcpServer{nullptr};
  WiFiClient client_server;
  WiFiUDP Udp;

  std::vector<WiFiClient*> clients;
  std::vector<String> id_cmd;
  unsigned long go_fullstatus{};
  const unsigned long interval = 1 * 60 * 1000;

  unsigned long time_ping_pong{};
  unsigned long interval_ping_pong{3 * 60 * 1000};
  String fullStatus{};
  int cmd_id_count{1000};
  bool connect_server{false};
  bool serverOK {false};
  const String local_name = "ESPMAIN";
}; 

#endif 
