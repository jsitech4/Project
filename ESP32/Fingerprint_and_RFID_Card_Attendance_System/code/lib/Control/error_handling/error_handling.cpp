#include <Arduino.h>
#include "error_handling.h"
#include "esp_system.h"

namespace error_handling
{
  static bool watchdogError = false;
  static bool codeError = false;
  static String lastError = "None";

  static bool isWatchdogReset(esp_reset_reason_t reason)
  {
    return reason == ESP_RST_WDT ||
           reason == ESP_RST_INT_WDT ||
           reason == ESP_RST_TASK_WDT;
  }

  void begin()
  {
    esp_reset_reason_t reason = esp_reset_reason();

    watchdogError = isWatchdogReset(reason);
    codeError = false;

    if (watchdogError)
      lastError = "Watchdog reset detected";
    else
      lastError = "None";
  }

  void update()
  {
  }

  void setCodeError(bool state)
  {
    codeError = state;

    if (state)
      lastError = "Code error flag set";
    else if (watchdogError)
      lastError = "Watchdog reset detected";
    else
      lastError = "None";
  }

  void setError(const String &message)
  {
    codeError = true;

    if (message.length() > 0)
      lastError = message;
    else
      lastError = "Code error flag set";
  }

  void clearCodeError()
  {
    codeError = false;

    if (watchdogError)
      lastError = "Watchdog reset detected";
    else
      lastError = "None";
  }


  void setBatteryError(bool state)
  {
    if (state)
    {
      setError("Low battery");
      return;
    }

    if (codeError && lastError == "Low battery")
      clearCodeError();
  }

  bool hasError()
  {
    return watchdogError || codeError;
  }

  bool hasWatchdogError()
  {
    return watchdogError;
  }

  bool hasCodeError()
  {
    return codeError;
  }

  String getLastError()
  {
    return lastError;
  }
}
