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

  // false = NEPA/GRID selected, true = inverter selected.
  // Startup is intentionally safe: all loads OFF and source preselected to NEPA.
  bool onInverterState[6] = {false, false, false, false, false, false};
  bool loadEnabledState[6] = {false, false, false, false, false, false};
  bool appliedOnInverterState[6] = {false, false, false, false, false, false};

  bool dirty = true;

  const unsigned long sourceSwitchDeadTimeMs = 80;

  void applyRelay(int index)
  {
    if (!gpio_expander::isReady())
      return;

    // Hardware convention used by the project:
    // source relay LOW  = inverter side
    // source relay HIGH = NEPA/grid side
    bool sourceRelayEnergized = !onInverterState[index];
    bool loadRelayEnergized = loadEnabledState[index];
    bool sourceChanged = appliedOnInverterState[index] != onInverterState[index];

    if (sourceChanged && loadRelayEnergized)
    {
      // Break before source transfer, then remake the load relay.
      // This keeps the source/load pair synchronized and avoids switching a live load directly.
      gpio_expander::digitalWrite(loadPins[index], false);
      delay(sourceSwitchDeadTimeMs);
      gpio_expander::digitalWrite(sourcePins[index], sourceRelayEnergized);
      delay(sourceSwitchDeadTimeMs);
      gpio_expander::digitalWrite(loadPins[index], true);
    }
    else
    {
      if (!loadRelayEnergized)
        gpio_expander::digitalWrite(loadPins[index], false);

      gpio_expander::digitalWrite(sourcePins[index], sourceRelayEnergized);
      gpio_expander::digitalWrite(loadPins[index], loadRelayEnergized);
    }

    appliedOnInverterState[index] = onInverterState[index];
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

  void setAllNEPA()
  {
    for (int i = 0; i < 6; i++)
      setRelay(i, false);
  }

  void setAllPHCN()
  {
    setAllNEPA();
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
