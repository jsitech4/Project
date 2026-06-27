#ifndef BATTERY_LEVEL_H
#define BATTERY_LEVEL_H

#include <Arduino.h>

namespace battery_level
{
  void begin();
  void update();

  float getVoltage();
  uint8_t getPercentage();
  int getRaw();

  bool isLow();
  bool shouldSleep();
  void sleepNow();
}

#endif
