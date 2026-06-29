#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

namespace wifi_manager
{
  void begin();
  void update();
  bool isConnected();
  bool isReady();
  String getSSID();
  String getPassword();
  String getIpString();
  IPAddress getIp();
}

#endif
