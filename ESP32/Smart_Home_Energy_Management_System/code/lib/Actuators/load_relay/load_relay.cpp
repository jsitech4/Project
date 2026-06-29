#include <Arduino.h>
#include "Pins.h"
#include "gpio_expander/gpio_expander.h"
#include "load_relay.h"

namespace load_relay
{
  const uint8_t sourcePins[6] = {
      Pins::EXP_RELAY_1_SOURCE,
      Pins::EXP_RELAY_2_SOURCE,
      Pins::EXP_RELAY_3_SOURCE,
      Pins::EXP_RELAY_4_SOURCE,
      Pins::EXP_RELAY_5_SOURCE,
      Pins::EXP_RELAY_6_SOURCE};

  const uint8_t loadPins[6] = {
      Pins::EXP_RELAY_1_LOAD,
      Pins::EXP_RELAY_2_LOAD,
      Pins::EXP_RELAY_3_LOAD,
      Pins::EXP_RELAY_4_LOAD,
      Pins::EXP_RELAY_5_LOAD,
      Pins::EXP_RELAY_6_LOAD};

  // Fail-safe startup: loads stay OFF until load_manager makes a valid source plan.
  bool onInverterState[6] = {false, false, false, false, false, false};
  bool loadEnabledState[6] = {false, false, false, false, false, false};

  bool dirty = true;
  const unsigned int relaySettleDelayMs = 20;

  void applyRelay(int index)
  {
    if (!gpio_expander::isReady())
      return;

    // In this board wiring, source relay energized selects NEPA/PHCN,
    // and de-energized selects inverter. Keep this single place if hardware changes.
    bool sourceRelayEnergizedForNEPA = !onInverterState[index];
    bool loadRelayEnergized = loadEnabledState[index];

    // Break-before-make: drop the load before moving the source selector.
    gpio_expander::digitalWrite(loadPins[index], false);
    delay(relaySettleDelayMs);

    gpio_expander::digitalWrite(sourcePins[index], sourceRelayEnergizedForNEPA);
    delay(relaySettleDelayMs);

    gpio_expander::digitalWrite(loadPins[index], loadRelayEnergized);
  }

  void begin()
  {
    if (!gpio_expander::isReady())
      return;

    for (int i = 0; i < 6; i++)
    {
      gpio_expander::pinMode(sourcePins[i], OUTPUT);
      gpio_expander::pinMode(loadPins[i], OUTPUT);
      gpio_expander::digitalWrite(loadPins[i], false);
    }

    dirty = true;
    update();
  }

  void update()
  {
    if (!dirty)
      return;

    if (!gpio_expander::isReady())
      return;

    for (int i = 0; i < 6; i++)
      applyRelay(i);

    dirty = false;
  }

  void setRelay(int relay, bool onInverter)
  {
    if (relay < 0 || relay > 5)
      return;

    if (onInverterState[relay] != onInverter)
    {
      onInverterState[relay] = onInverter;
      dirty = true;
    }
  }

  void setLoadEnabled(int relay, bool enabled)
  {
    if (relay < 0 || relay > 5)
      return;

    if (loadEnabledState[relay] != enabled)
    {
      loadEnabledState[relay] = enabled;
      dirty = true;
    }
  }

  void setAllInverter()
  {
    for (int i = 0; i < 6; i++)
      setRelay(i, true);
  }

  void setAllPHCN()
  {
    for (int i = 0; i < 6; i++)
      setRelay(i, false);
  }

  void enableAllLoads()
  {
    for (int i = 0; i < 6; i++)
      setLoadEnabled(i, true);
  }

  void disableAllLoads()
  {
    for (int i = 0; i < 6; i++)
      setLoadEnabled(i, false);
  }

  bool isOnInverter(int relay)
  {
    if (relay < 0 || relay > 5)
      return false;

    return onInverterState[relay];
  }

  bool isLoadEnabled(int relay)
  {
    if (relay < 0 || relay > 5)
      return false;

    return loadEnabledState[relay];
  }

  bool getRelay1() { return onInverterState[0]; }
  bool getRelay2() { return onInverterState[1]; }
  bool getRelay3() { return onInverterState[2]; }
  bool getRelay4() { return onInverterState[3]; }
  bool getRelay5() { return onInverterState[4]; }
  bool getRelay6() { return onInverterState[5]; }
}
