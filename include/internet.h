#ifndef INTERNET_H
#define INTERNET_H
#include <WiFi.h>
#include <Arduino.h>
#include "encryption.h"
#include <skeleton.h>
#include <config.h>
#include <clock.h>
#include <WebServer.h>
#include "SyncBuilder.h"
#include "authorization.h"

class Internet : public Encryption, public Config
{
public:
  explicit Internet(CLOCK &myclock);
  bool connect();
  void startTCPSerwer();
  void processServerResponse();

private:
  // void ping_pong();

  void communication_udp();

  // Порт TCP сервера
  const uint16_t TCP_PORT = 9000;

  // TCP сервер
  WiFiServer *tcpServer{nullptr};

  WiFiUDP Udp;
  ClientTCP serwer;
  std::deque<ClientTCP *> clients;

  const String local_name = "MIDCAMAINU";
};

#endif
