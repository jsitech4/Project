#ifndef LOAD_RELAY_H
#define LOAD_RELAY_H

#include <Arduino.h>

namespace load_relay
{
  void begin();
  void update();

  void setOn(bool state);
  void turnOn();
  void turnOff();

  void trip();
  void clearFault();

  bool isOn();
  bool isFault();
  bool getRequestedState();
}

#endif
