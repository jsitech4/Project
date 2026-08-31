#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <Arduino.h>
#include <esp_system.h>

namespace error_handling
{
  void begin();
  void update();

  void setCodeError(bool state);
  void clearCodeError();

  bool hasError();
  bool hasCodeError();
  bool watchdogResetOccurred();

  esp_reset_reason_t getResetReason();
}

#endif
