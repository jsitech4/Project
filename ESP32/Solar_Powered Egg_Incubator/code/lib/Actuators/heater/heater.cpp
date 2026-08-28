#include <Arduino.h>
#include "heater.h"
namespace heater
{
  static uint8_t relayPin = 255;
  static bool relayState = false, relayActiveLow = true;
  static void apply()
  {
    if (relayPin == 255)
      return;
    digitalWrite(relayPin, relayActiveLow ? relayState : !relayState);
  }
  void begin(uint8_t pin, bool activeLow)
  {
    relayPin = pin;
    relayActiveLow = activeLow;
    pinMode(relayPin, OUTPUT);
    relayState = false;
    apply();
  }
  void update() { apply(); }
  void setState(bool state)
  {
    relayState = state;
    apply();
  }
  void toggle() { setState(!relayState); }
  bool isOn() { return relayState; }
}
