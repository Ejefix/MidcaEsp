#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <SPIFFS.h>

class Config {
public:
  Config();
  String get_ssid();
  String get_password();
  String get_serverIP();
  String get_serverPort();
protected:
  bool updateConfig(const String &newSSID, const String &newPassword,
                    const String &newServerIP = "176.60.208.46", const String &newServerPort = "64530");
  bool begin();
private:

  const String pathSsid{ "/config_ssid.bin" };
  const String pathPassword{ "/config_Password.bin" };
  const String pathIP{ "/config_IP.bin" };
  const String pathPort{ "/config_Port.bin" };

  String ssid{};
  String password{};
  String serverIP{};
  String serverPort{};

  const String ssidDefault{ "Asterix" };
  const String passwordDefault{ "09090909" };
  const String serverIPDefault{ "176.60.208.46" };
  const String serverPortDefault{ "64530" };
  bool config{false};
};


#endif
