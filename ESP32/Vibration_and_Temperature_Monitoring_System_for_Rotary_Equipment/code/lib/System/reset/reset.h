#ifndef RESET_H
#define RESET_H

#include <Arduino.h>

namespace reset
{
  void begin();
  void update();

  void restart();
  void restartAfter(unsigned long delayMs);

  bool isRestartPending();
}

#endif
