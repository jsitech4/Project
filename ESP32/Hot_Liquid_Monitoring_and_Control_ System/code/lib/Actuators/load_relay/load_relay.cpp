#include <Arduino.h>
#include "Pins.h"
#include "load_relay.h"

namespace load_relay
{
  static const bool RELAY_ACTIVE_HIGH = true;

  static bool relayOn = false;
  static bool requestedState = true;
  static bool faultLatched = false;

  static void writeRelay(bool state)
  {
    relayOn = state;
    bool level = RELAY_ACTIVE_HIGH ? state : !state;
    Pins::writePin(Pins::RELAY, level);
  }

  void begin()
  {
    pinMode(Pins::RELAY, OUTPUT);
    faultLatched = false;
    requestedState = true;
    writeRelay(requestedState);
  }

  void update()
  {
    if (faultLatched)
    {
      writeRelay(false);
      return;
    }

    writeRelay(requestedState);
  }

  void setOn(bool state)
  {
    requestedState = state;

    if (!faultLatched)
    {
      writeRelay(requestedState);
    }
  }

  void turnOn()
  {
    setOn(true);
  }

  void turnOff()
  {
    setOn(false);
  }

  void trip()
  {
    faultLatched = true;
    writeRelay(false);
  }

  void clearFault()
  {
    faultLatched = false;
    writeRelay(requestedState);
  }

  bool isOn()
  {
    return relayOn;
  }

  bool isFault()
  {
    return faultLatched;
  }

  bool getRequestedState()
  {
    return requestedState;
  }
}
