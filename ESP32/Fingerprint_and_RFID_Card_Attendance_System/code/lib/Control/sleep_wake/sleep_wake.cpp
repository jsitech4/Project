#include <Arduino.h>
#include "sleep_wake.h"
#include "esp_sleep.h"

namespace sleep_wake
{
  static bool sleepRequested = false;
  static bool wokeFromDeepSleep = false;
  static String wakeReason = "Power on reset";

  static String decodeWakeReason(esp_sleep_wakeup_cause_t cause)
  {
    switch (cause)
    {
      case ESP_SLEEP_WAKEUP_UNDEFINED:
        return "Power on reset";

      case ESP_SLEEP_WAKEUP_EXT0:
        return "Wake from EXT0 pin";

      case ESP_SLEEP_WAKEUP_EXT1:
        return "Wake from EXT1 pin";

      case ESP_SLEEP_WAKEUP_TIMER:
        return "Wake from timer";

      case ESP_SLEEP_WAKEUP_TOUCHPAD:
        return "Wake from touchpad";

      case ESP_SLEEP_WAKEUP_ULP:
        return "Wake from ULP";

      default:
        return "Wake from other source";
    }
  }

  void begin()
  {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    wokeFromDeepSleep = cause != ESP_SLEEP_WAKEUP_UNDEFINED;
    wakeReason = decodeWakeReason(cause);
    sleepRequested = false;
  }

  void update()
  {
    if (!sleepRequested)
      return;

    sleepRequested = false;
    sleepNow();
  }

  void requestSleep(bool state)
  {
    sleepRequested = state;
  }

  void cancelSleep()
  {
    sleepRequested = false;
  }

  void sleepNow()
  {
    esp_deep_sleep_start();
  }

  bool isSleepRequested()
  {
    return sleepRequested;
  }

  bool wokeFromSleep()
  {
    return wokeFromDeepSleep;
  }

  String getWakeReason()
  {
    return wakeReason;
  }
}
