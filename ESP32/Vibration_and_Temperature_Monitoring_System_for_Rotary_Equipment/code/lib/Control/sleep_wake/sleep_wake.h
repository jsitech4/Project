#ifndef SLEEP_WAKE_H
#define SLEEP_WAKE_H

#include <Arduino.h>
#include <esp_sleep.h>

namespace sleep_wake
{
  void begin();
  void update();

  void requestSleep(bool state);
  void sleepNow();
  void cancelSleep();

  bool isSleepRequested();
  bool wokeFromSleep();

  esp_sleep_wakeup_cause_t getWakeupCause();
}

#endif
