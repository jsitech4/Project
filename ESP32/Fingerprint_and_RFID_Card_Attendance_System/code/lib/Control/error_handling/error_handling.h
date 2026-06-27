#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <Arduino.h>

namespace error_handling
{
  void begin();
  void update();

  void setCodeError(bool state);
  void setError(const String &message);
  void clearCodeError();
  void setBatteryError(bool state);

  bool hasError();
  bool hasWatchdogError();
  bool hasCodeError();

  String getLastError();
}

#endif
