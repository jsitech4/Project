#include <Arduino.h>
#include <RTClib.h>
#include "rtc_clock.h"
#include "Pins.h"

//========================================================
// Set to 1 ONLY when you want to update the RTC.
// Upload once, then change back to 0.
//========================================================
#define SET_RTC_ONCE 1

namespace rtc_clock
{
  static RTC_DS3231 rtcChip;
  static bool ready = false;

  static String cachedTimestamp = "0000-00-00 00:00:00";
  static String cachedDate = "0000-00-00";
  static String cachedTime = "12:00:00 AM";

  static unsigned long lastRefresh = 0;
  static const unsigned long refreshInterval = 1000;

  static String twoDigits(int value)
  {
    if (value < 10)
      return "0" + String(value);

    return String(value);
  }

  static String millisTimestamp()
  {
    unsigned long seconds = millis() / 1000;

    unsigned long h = (seconds / 3600) % 24;
    unsigned long m = (seconds / 60) % 60;
    unsigned long s = seconds % 60;

    return "PROTO-0000 " +
           twoDigits((int)h) + ":" +
           twoDigits((int)m) + ":" +
           twoDigits((int)s);
  }

  static void refreshNow()
  {
    if (!ready)
    {
      cachedTimestamp = millisTimestamp();
      cachedDate = "0000-00-00";
      cachedTime = "12:00:00 AM";
      return;
    }

    DateTime now = rtcChip.now();

    cachedDate =
        twoDigits(now.day()) + "/" +
        twoDigits(now.month()) + "/" +
        String(now.year());

    int hour = now.hour();
    String ampm = "AM";

    if (hour >= 12)
      ampm = "PM";

    if (hour == 0)
      hour = 12;
    else if (hour > 12)
      hour -= 12;

    cachedTime =
        twoDigits(hour) + ":" +
        twoDigits(now.minute()) + ":" +
        twoDigits(now.second()) +
        ampm;

    cachedTimestamp =
        cachedDate + " " + cachedTime;
  }

  void begin()
  {
    Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

    ready = rtcChip.begin();

    if (!ready)
    {
      Serial.println("RTC NOT FOUND!");
      refreshNow();
      return;
    }

#if SET_RTC_ONCE

    Serial.println();
    Serial.println("==============================");
    Serial.println("SETTING RTC");
    Serial.println("==============================");

    rtcChip.adjust(DateTime(F(__DATE__), F(__TIME__)));

#else

    if (rtcChip.lostPower())
    {
      Serial.println("RTC LOST POWER");
      rtcChip.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

#endif

    refreshNow();

    Serial.print("RTC READY : ");
    Serial.println(cachedTimestamp);

    lastRefresh = millis();
  }

  void update()
  {
    if (millis() - lastRefresh < refreshInterval)
      return;

    lastRefresh = millis();

    refreshNow();
  }

  bool isReady()
  {
    return ready;
  }

  String getTimestamp()
  {
    return cachedTimestamp;
  }

  String getDate()
  {
    return cachedDate;
  }

  String getTime()
  {
    return cachedTime;
  }
}
