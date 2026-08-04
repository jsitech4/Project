#include <Arduino.h>
#include "reset.h"

namespace reset
{
  static bool restartPending = false;
  static unsigned long restartTime = 0;

  void begin()
  {
    restartPending = false;
    restartTime = 0;
  }

  void update()
  {
    if (restartPending && millis() >= restartTime)
    {
      ESP.restart();
    }
  }

  void restart()
  {
    ESP.restart();
  }

  void restartAfter(unsigned long delayMs)
  {
    restartPending = true;
    restartTime = millis() + delayMs;
  }

  bool isRestartPending()
  {
    return restartPending;
  }
}
