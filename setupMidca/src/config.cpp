#include "config.h"
#include "setupesp.h"

Config::Config() {
  begin();
}

bool Config::begin() {
  config = SETUP_ESP::readToFile(pathSsid, ssid)
           && SETUP_ESP::readToFile(pathPassword, password)
           && SETUP_ESP::readToFile(pathIP, serverIP)
           && SETUP_ESP::readToFile(pathPort, serverPort);
  return config;
}

String Config::get_ssid() {
  if (config) {
    return ssid;
  } else {
    return ssidDefault;
  }
}

String Config::get_password() {
  if (config) {
    return password;
  } else {
    return passwordDefault;
  }
}

String Config::get_serverIP() {

  if (config) {
    return serverIP;
  } else {
    return serverIPDefault;
  }
}

String Config::get_serverPort() {
  if (config) {
    return serverPort;
  } else {
    return serverPortDefault;
  }
}

bool Config::updateConfig(const String &newSSID, const String &newPassword,
                          const String &newServerIP, const String &newServerPort) {
  bool configNEW;
  configNEW = SETUP_ESP::saveToFile(pathSsid, newSSID)
              && SETUP_ESP::saveToFile(pathPassword, newPassword)
              && SETUP_ESP::saveToFile(pathIP, newServerIP)
              && SETUP_ESP::saveToFile(pathPort, newServerPort);
  if (configNEW) {
    ssid = newSSID;
    password = newPassword;
    serverIP = newServerIP;
    serverPort = newServerPort;
    Serial.println("[LOG] обновили Config");
    config = true;

  } else {
    SETUP_ESP::deleteFile(pathSsid);
    SETUP_ESP::deleteFile(pathPassword);
    SETUP_ESP::deleteFile(pathIP);
    SETUP_ESP::deleteFile(pathPort);
    Serial.println("❌ не удалось обновить Config");
    config = false;
  }
  return config;
}
