#include <Arduino.h>
#include <math.h>
#include "load_manager.h"
#include "load_relay/load_relay.h"
#include "config_manager/config_manager.h"
#include "pzem_sensor/pzem_sensor.h"
#include "nepa_sense/nepa_sense.h"
#include "inverter_sense/inverter_sense.h"

namespace load_manager
{
  bool onInverter[6] = {false, false, false, false, false, false};
  bool loadEnabled[6] = {false, false, false, false, false, false};

  float loadRatio = 0.0f;
  float fuzzyRisk = 0.0f;

  int currentLoad = 0;
  int configuredTotalLoad = 0;
  int effectiveLimit = 0;
  int effectiveInverterLimit = 0;
  int effectiveSystemLimit = 0;

  const char *decisionText = "STARTING";

  unsigned long lastUpdate = 0;
  const unsigned long updateInterval = 500;

  float clampFloat(float value, float low, float high)
  {
    if (value < low)
      return low;

    if (value > high)
      return high;

    return value;
  }

  float risingMembership(float x, float a, float b)
  {
    if (x <= a)
      return 0.0f;

    if (x >= b)
      return 1.0f;

    return (x - a) / (b - a);
  }

  float fallingMembership(float x, float a, float b)
  {
    if (x <= a)
      return 1.0f;

    if (x >= b)
      return 0.0f;

    return (b - x) / (b - a);
  }

  int calculateConfiguredTotalLoad()
  {
    int total = 0;

    for (int i = 0; i < 6; i++)
      total += config_manager::getRelayPower(i);

    return total;
  }

  int applyMargin(int ratingWatts)
  {
    long result = ((long)ratingWatts * (long)config_manager::getLoadMarginPercent()) / 100L;

    if (result < 1)
      result = 1;

    if (result > 50000)
      result = 50000;

    return (int)result;
  }

  int countMaskBits(int mask)
  {
    int count = 0;

    for (int i = 0; i < 6; i++)
    {
      if (mask & (1 << i))
        count++;
    }

    return count;
  }

  int sumMaskPower(int mask)
  {
    int sum = 0;

    for (int i = 0; i < 6; i++)
    {
      if (mask & (1 << i))
        sum += config_manager::getRelayPower(i);
    }

    return sum;
  }

  int chooseLowestPowerPriorityMask(int availableMask, int limit)
  {
    if (limit <= 0 || availableMask == 0)
      return 0;

    int bestMask = 0;
    long bestScore = -2147483647L;

    for (int mask = 0; mask < 64; mask++)
    {
      if ((mask & availableMask) != mask)
        continue;

      int sum = 0;
      int selectedCount = 0;
      long lowPowerPriority = 0;

      for (int i = 0; i < 6; i++)
      {
        if (mask & (1 << i))
        {
          int power = config_manager::getRelayPower(i);
          sum += power;
          selectedCount++;
          lowPowerPriority += (20000L - (long)power);
        }
      }

      if (sum > limit)
        continue;

      // Lower-power relays win first, then more enabled loads, then better use of the available source.
      long score = (lowPowerPriority * 100L) + (selectedCount * 10000L) + sum;

      if (score > bestScore)
      {
        bestScore = score;
        bestMask = mask;
      }
    }

    return bestMask;
  }

  void clearPlan()
  {
    for (int i = 0; i < 6; i++)
    {
      loadEnabled[i] = false;
      onInverter[i] = false;
    }
  }

  void enableMask(int mask, bool inverterSource)
  {
    for (int i = 0; i < 6; i++)
    {
      if (mask & (1 << i))
      {
        loadEnabled[i] = true;
        onInverter[i] = inverterSource;
      }
    }
  }

  void applyDecision()
  {
    for (int i = 0; i < 6; i++)
    {
      load_relay::setLoadEnabled(i, loadEnabled[i]);
      load_relay::setRelay(i, onInverter[i]);
    }
  }

  void updateFuzzyRisk()
  {
    if (effectiveLimit < 1)
      effectiveLimit = 1;

    loadRatio = (float)currentLoad / (float)effectiveLimit;

    float safeLoad = fallingMembership(loadRatio, 0.70f, 0.90f);

    float warningLoad = 1.0f - fabs(loadRatio - 0.95f) / 0.25f;
    warningLoad = clampFloat(warningLoad, 0.0f, 1.0f);

    float overloadLoad = risingMembership(loadRatio, 0.95f, 1.10f);

    fuzzyRisk =
        (safeLoad * 0.10f) +
        (warningLoad * 0.55f) +
        (overloadLoad * 1.00f);

    fuzzyRisk = clampFloat(fuzzyRisk, 0.0f, 1.0f);
  }

  bool allConfiguredLoadsCovered()
  {
    int enabledPower = 0;

    for (int i = 0; i < 6; i++)
    {
      if (loadEnabled[i])
        enabledPower += config_manager::getRelayPower(i);
    }

    return enabledPower >= configuredTotalLoad;
  }

  void planSources(bool nepaAvailable, bool inverterAvailable)
  {
    const int allLoadsMask = 0x3F;

    clearPlan();

    if (!nepaAvailable && !inverterAvailable)
    {
      decisionText = "NO SOURCE";
      return;
    }

    if (nepaAvailable)
    {
      // NEPA/PHCN is always the primary source. Fill it first with lower-power loads.
      int nepaMask = chooseLowestPowerPriorityMask(allLoadsMask, effectiveSystemLimit);
      enableMask(nepaMask, false);

      int remainingMask = allLoadsMask & ~nepaMask;

      if (remainingMask != 0 && inverterAvailable)
      {
        int inverterMask = chooseLowestPowerPriorityMask(remainingMask, effectiveInverterLimit);
        enableMask(inverterMask, true);
      }

      if (allConfiguredLoadsCovered())
      {
        if (remainingMask == 0)
          decisionText = inverterAvailable ? "NEPA PRIORITY" : "NEPA ONLY";
        else
          decisionText = "NEPA + INVERTER";
      }
      else
      {
        decisionText = inverterAvailable ? "LIMITED SOURCES" : "NEPA LOAD SHED";
      }

      return;
    }

    // If NEPA is down, use inverter as backup. Lower-power loads receive priority.
    if (inverterAvailable)
    {
      int inverterMask = chooseLowestPowerPriorityMask(allLoadsMask, effectiveInverterLimit);
      enableMask(inverterMask, true);

      if (allConfiguredLoadsCovered())
        decisionText = "INVERTER ONLY";
      else
        decisionText = "INV LOAD SHED";
    }
  }

  void begin()
  {
    clearPlan();
    decisionText = "READY";
    applyDecision();
  }

  void update()
  {
    unsigned long now = millis();

    if (now - lastUpdate < updateInterval)
      return;

    lastUpdate = now;

    configuredTotalLoad = calculateConfiguredTotalLoad();

    if (pzem_sensor::hasValidData())
      currentLoad = pzem_sensor::getTotalPower();
    else
      currentLoad = configuredTotalLoad;

    effectiveInverterLimit = applyMargin(config_manager::getInverterPower());
    effectiveSystemLimit = applyMargin(config_manager::getSystemPower());

    bool nepaAvailable = nepa_sense::isAvailable();
    bool inverterAvailable = inverter_sense::isAvailable();

    effectiveLimit = nepaAvailable ? effectiveSystemLimit : effectiveInverterLimit;
    updateFuzzyRisk();

    planSources(nepaAvailable, inverterAvailable);
    applyDecision();
  }

  bool isOnInverter(uint8_t relay)
  {
    if (relay >= 6)
      return false;

    return onInverter[relay];
  }

  bool isLoadEnabled(uint8_t relay)
  {
    if (relay >= 6)
      return false;

    return loadEnabled[relay];
  }

  float getLoadRatio()
  {
    return loadRatio;
  }

  float getFuzzyRisk()
  {
    return fuzzyRisk;
  }

  int getCurrentLoad()
  {
    return currentLoad;
  }

  int getConfiguredTotalLoad()
  {
    return configuredTotalLoad;
  }

  int getEffectiveLimit()
  {
    return effectiveLimit;
  }

  int getEffectiveInverterLimit()
  {
    return effectiveInverterLimit;
  }

  int getEffectiveSystemLimit()
  {
    return effectiveSystemLimit;
  }

  const char *getDecisionText()
  {
    return decisionText;
  }
}
