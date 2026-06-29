#include <Arduino.h>
#include <WiFi.h>
#include "wifi_manager.h"
#include "sd_card/sd_card.h"

namespace wifi_manager
{
  static bool ready = false;
  static String ssid = "AttendanceSystem";
  static String password = "12345678";
  static IPAddress ip;

  static String cleanText(String value)
  {
    value.trim();
    value.replace("\r", "");
    value.replace("\n", "");
    return value;
  }

  void begin()
  {
    ssid = cleanText(sd_card::getConfigValue("ap_ssid", "AttendanceSystem"));
    password = cleanText(sd_card::getConfigValue("ap_password", "12345678"));

    if (ssid.length() < 1 || ssid.length() > 32)
      ssid = "AttendanceSystem";

    if (password.length() < 8 || password.length() > 63)
      password = "12345678";

    WiFi.mode(WIFI_OFF);
    delay(100);

    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);

    ready = WiFi.softAP(ssid.c_str(), password.c_str());
    ip = WiFi.softAPIP();
    if (!ready)
    {
      return;
    }
  }

  void update()
  {
    if (!ready)
    {
      return;
    }
  }
  bool isConnected()
  {
    return WiFi.softAPgetStationNum() > 0;
  }

  bool isReady()
  {
    return ready;
  }

  String getSSID()
  {
    return ssid;
  }

  String getPassword()
  {
    return password;
  }

  String getIpString()
  {
    if (!ready)
      return "0.0.0.0";

    return ip.toString();
  }

  IPAddress getIp()
  {
    return ip;
  }
}
