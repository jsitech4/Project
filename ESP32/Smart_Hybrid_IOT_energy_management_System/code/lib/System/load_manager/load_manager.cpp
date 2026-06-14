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
  int effectiveLimit = 0;

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

  void setAllLoads(bool enabled, bool inverter)
  {
    for (int i = 0; i < 6; i++)
    {
      loadEnabled[i] = enabled;
      onInverter[i] = inverter;
    }
  }

  int chooseLowPowerPriorityMask(int limit)
  {
    int power[6];
    int highestPower = 1;

    for (int i = 0; i < 6; i++)
    {
      power[i] = config_manager::getRelayPower(i);
      if (power[i] > highestPower)
        highestPower = power[i];
    }

    int bestMask = 0;
    long bestScore = -2147483647L;

    for (int mask = 0; mask < 64; mask++)
    {
      int sum = 0;
      int count = 0;
      long lowPowerPriority = 0;

      for (int i = 0; i < 6; i++)
      {
        if (mask & (1 << i))
        {
          sum += power[i];
          count++;
          lowPowerPriority += (long)(highestPower + 1000 - power[i]);
        }
      }

      if (sum > limit)
        continue;

      // Priority order:
      // 1. Keep more loads alive.
      // 2. Prefer lower-power relays over heavy loads.
      // 3. Use reasonable available capacity after the first two priorities.
      long score = ((long)count * 100000L) + (lowPowerPriority * 20L) + sum;

      if (score > bestScore)
      {
        bestScore = score;
        bestMask = mask;
      }
    }

    return bestMask;
  }

  void chooseLoadsForSingleSource(int limit, bool inverter)
  {
    int bestMask = chooseLowPowerPriorityMask(limit);

    for (int i = 0; i < 6; i++)
    {
      loadEnabled[i] = (bestMask & (1 << i)) != 0;
      onInverter[i] = inverter;
    }
  }

  void chooseInverterAssistWhileNepaCarriesRest(int limit)
  {
    int bestMask = chooseLowPowerPriorityMask(limit);

    for (int i = 0; i < 6; i++)
    {
      loadEnabled[i] = true;

      // Low-power selected relays are allowed on inverter.
      // Everything else stays on NEPA, because NEPA is the priority source.
      onInverter[i] = (bestMask & (1 << i)) != 0;
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

  void begin()
  {
    setAllLoads(false, false);
    decisionText = "READY";
    applyDecision();
  }

  void update()
  {
    unsigned long now = millis();

    if (now - lastUpdate < updateInterval)
      return;

    lastUpdate = now;

    currentLoad = pzem_sensor::getTotalPower();

    int inverterLimit = config_manager::getInverterPower();
    int margin = config_manager::getLoadMarginPercent();

    effectiveLimit = (inverterLimit * margin) / 100;

    if (effectiveLimit < 1)
      effectiveLimit = 1;

    loadRatio = (float)currentLoad / (float)effectiveLimit;
    updateFuzzyRisk();

    // bool nepaAvailable = nepa_sense::isAvailable();
    // bool inverterAvailable = inverter_sense::isAvailable();
    bool nepaAvailable = true;
    bool inverterAvailable = false;
    bool sensorValid = pzem_sensor::hasValidData();
    bool highRisk = sensorValid && (loadRatio >= 1.0f || (loadRatio >= 0.90f && fuzzyRisk >= 0.55f));

    if (!nepaAvailable && !inverterAvailable)
    {
      setAllLoads(false, false);
      decisionText = "NO SOURCE";
    }
    else if (nepaAvailable && inverterAvailable)
    {
      if (highRisk)
      {
        chooseInverterAssistWhileNepaCarriesRest(effectiveLimit);
        decisionText = "NEPA ASSIST";
      }
      else
      {
        // NEPA is the preferred/default source whenever it is healthy.
        setAllLoads(true, false);
        decisionText = sensorValid ? "NEPA PRIORITY" : "PZEM ERROR";
      }
    }
    else if (nepaAvailable && !inverterAvailable)
    {
      // No inverter available: still make an intelligent NEPA-only configuration.
      // Lower configured relay powers are favored before heavy loads.
      chooseLoadsForSingleSource(effectiveLimit, false);
      decisionText = "NEPA ONLY OPT";
    }
    else
    {
      // NEPA/grid unavailable: run only what the inverter can carry.
      chooseLoadsForSingleSource(effectiveLimit, true);
      decisionText = sensorValid ? "INVERTER ONLY" : "PZEM ERROR";
    }

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

  int getEffectiveLimit()
  {
    return effectiveLimit;
  }

  const char *getDecisionText()
  {
    return decisionText;
  }
}
