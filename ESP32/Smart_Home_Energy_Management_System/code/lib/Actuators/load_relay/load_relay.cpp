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

  // Desired final states requested by load_manager.
  // onInverterState=false means NEPA/PHCN source relay is selected.
  bool onInverterState[6] = {false, false, false, false, false, false};
  bool loadEnabledState[6] = {false, false, false, false, false, false};

  // Last states fully applied to the expander. These prevent repeated relay writes.
  bool appliedOnInverterState[6] = {false, false, false, false, false, false};
  bool appliedLoadEnabledState[6] = {false, false, false, false, false, false};
  bool appliedKnown[6] = {false, false, false, false, false, false};

  bool dirty = true;

  const unsigned long relaySettleDelayMs = 10;

  int activeRelay = -1;
  uint8_t activeStage = 0;
  unsigned long stageStart = 0;
  bool activeTargetOnInverter = false;
  bool activeTargetLoadEnabled = false;

  bool sourceRelayLevelForValue(bool onInverter)
  {
    // In this board wiring, source relay energized selects NEPA/PHCN,
    // and de-energized selects inverter. Keep this single place if hardware changes.
    return !onInverter;
  }

  bool relayNeedsApply(int index)
  {
    if (!appliedKnown[index])
      return true;

    return appliedOnInverterState[index] != onInverterState[index] ||
           appliedLoadEnabledState[index] != loadEnabledState[index];
  }

  int findNextRelayToApply(int startIndex)
  {
    for (int offset = 0; offset < 6; offset++)
    {
      int index = (startIndex + offset) % 6;
      if (relayNeedsApply(index))
        return index;
    }

    return -1;
  }

  void finishIfClean()
  {
    for (int i = 0; i < 6; i++)
    {
      if (relayNeedsApply(i))
      {
        dirty = true;
        return;
      }
    }

    dirty = false;
  }

  void startApplyingRelay(int index)
  {
    activeRelay = index;
    activeStage = 0;
    stageStart = millis();
    activeTargetOnInverter = onInverterState[index];
    activeTargetLoadEnabled = loadEnabledState[index];
  }

  void serviceActiveRelay()
  {
    if (activeRelay < 0)
      return;

    unsigned long now = millis();

    switch (activeStage)
    {
    case 0:
      // Break-before-make: open the load relay first and return immediately.
      gpio_expander::digitalWrite(loadPins[activeRelay], false);
      stageStart = now;
      activeStage = 1;
      return;

    case 1:
      if (now - stageStart < relaySettleDelayMs)
        return;

      gpio_expander::digitalWrite(sourcePins[activeRelay], sourceRelayLevelForValue(activeTargetOnInverter));
      stageStart = now;
      activeStage = 2;
      return;

    case 2:
    {
      if (now - stageStart < relaySettleDelayMs)
        return;

      bool targetChangedWhileSwitching =
          activeTargetOnInverter != onInverterState[activeRelay] ||
          activeTargetLoadEnabled != loadEnabledState[activeRelay];

      if (targetChangedWhileSwitching)
      {
        // Keep the load safely OFF if the plan changed during the break-before-make window.
        // The next non-blocking pass will move the source selector to the new target.
        gpio_expander::digitalWrite(loadPins[activeRelay], false);
        appliedOnInverterState[activeRelay] = activeTargetOnInverter;
        appliedLoadEnabledState[activeRelay] = false;
      }
      else
      {
        gpio_expander::digitalWrite(loadPins[activeRelay], activeTargetLoadEnabled);
        appliedOnInverterState[activeRelay] = activeTargetOnInverter;
        appliedLoadEnabledState[activeRelay] = activeTargetLoadEnabled;
      }

      appliedKnown[activeRelay] = true;

      activeRelay = -1;
      activeStage = 0;
      finishIfClean();
      return;
    }

    default:
      activeRelay = -1;
      activeStage = 0;
      dirty = true;
      return;
    }
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
      appliedKnown[i] = false;
    }

    dirty = true;
    activeRelay = -1;
    activeStage = 0;
  }

  void update()
  {
    if (!gpio_expander::isReady())
      return;

    if (activeRelay >= 0)
    {
      serviceActiveRelay();
      return;
    }

    if (!dirty)
      return;

    int nextRelay = findNextRelayToApply(0);

    if (nextRelay < 0)
    {
      dirty = false;
      return;
    }

    startApplyingRelay(nextRelay);
    serviceActiveRelay();
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

  bool isBusy()
  {
    return dirty || activeRelay >= 0;
  }

  bool getRelay1() { return onInverterState[0]; }
  bool getRelay2() { return onInverterState[1]; }
  bool getRelay3() { return onInverterState[2]; }
  bool getRelay4() { return onInverterState[3]; }
  bool getRelay5() { return onInverterState[4]; }
  bool getRelay6() { return onInverterState[5]; }
}
