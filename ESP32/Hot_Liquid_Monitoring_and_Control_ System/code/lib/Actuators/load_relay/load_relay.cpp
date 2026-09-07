#include <Arduino.h>
#include "Pins.h"
#include "load_relay.h"

namespace load_relay
{
  static const bool RELAY_ACTIVE_HIGH = true;

  static bool relayOn = false;
  static bool requestedState = false;

  static void writeRelay(bool state)
  {
    relayOn = state;
    bool level = RELAY_ACTIVE_HIGH ? state : !state;
    Pins::writePin(Pins::RELAY, level);
  }

  void begin()
  {
    pinMode(Pins::RELAY, OUTPUT);
    requestedState = false;
    writeRelay(requestedState);
  }

  void update()
  {
    writeRelay(requestedState);
  }

  void setOn(bool state)
  {
    requestedState = state;
    writeRelay(requestedState);
  }

  void turnOn()
  {
    setOn(true);
  }

  void turnOff()
  {
    setOn(false);
  }

  bool isOn()
  {
    return relayOn;
  }

  bool getRequestedState()
  {
    return requestedState;
  }
}
