#include <Arduino.h>
#include <esp_sleep.h>
#include "sleep_wake.h"

namespace sleep_wake
{
  static bool sleepRequested = false;
  static bool wokeFromDeepSleep = false;
  static esp_sleep_wakeup_cause_t wakeupCause = ESP_SLEEP_WAKEUP_UNDEFINED;

  void begin()
  {
    wakeupCause = esp_sleep_get_wakeup_cause();
    wokeFromDeepSleep = wakeupCause != ESP_SLEEP_WAKEUP_UNDEFINED;
    sleepRequested = false;
  }

  void update()
  {
    if (sleepRequested)
    {
      sleepNow();
    }
  }

  void requestSleep(bool state)
  {
    sleepRequested = state;
  }

  void sleepNow()
  {
    delay(50);
    esp_deep_sleep_start();
  }

  void cancelSleep()
  {
    sleepRequested = false;
  }

  bool isSleepRequested()
  {
    return sleepRequested;
  }

  bool wokeFromSleep()
  {
    return wokeFromDeepSleep;
  }

  esp_sleep_wakeup_cause_t getWakeupCause()
  {
    return wakeupCause;
  }
}
