#include <Arduino.h>
#include <esp_system.h>
#include "error_handling.h"

namespace error_handling
{
  static bool watchdogError = false;
  static bool codeError = false;
  static esp_reset_reason_t lastResetReason = ESP_RST_UNKNOWN;

  void begin()
  {
    lastResetReason = esp_reset_reason();

    watchdogError = (lastResetReason == ESP_RST_WDT ||
                     lastResetReason == ESP_RST_TASK_WDT ||
                     lastResetReason == ESP_RST_INT_WDT);

    codeError = false;
  }

  void update()
  {
  }

  void setCodeError(bool state)
  {
    codeError = state;
  }

  void clearCodeError()
  {
    codeError = false;
  }

  bool hasError()
  {
    return watchdogError || codeError;
  }

  bool hasCodeError()
  {
    return codeError;
  }

  bool watchdogResetOccurred()
  {
    return watchdogError;
  }

  esp_reset_reason_t getResetReason()
  {
    return lastResetReason;
  }
}
