#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include <Arduino.h>

namespace led_indicator
{
  void begin();
  void update();

  void on();
  void off();
  void resume();

  bool isOn();
}

#endif
