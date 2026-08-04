#ifndef RTC_CLOCK_H
#define RTC_CLOCK_H
#include <Arduino.h>
namespace rtc_clock
{
  void begin();
  void update();

  bool isReady();

  String getTimestamp();
  String getDate();
  String getTime();
}
#endif
