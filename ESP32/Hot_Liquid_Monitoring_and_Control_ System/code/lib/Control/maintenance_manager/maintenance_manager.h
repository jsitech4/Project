#ifndef MAINTENANCE_MANAGER_H
#define MAINTENANCE_MANAGER_H

#include <Arduino.h>

namespace maintenance_manager
{
  enum RelayLatchMode
  {
    LATCH_OFF = 0,
    LATCH_AT_FULL = 1,
    LATCH_AT_LOW = 2
  };

  struct Snapshot
  {
    unsigned long uptimeMs;
    float temperatureC;
    float distanceCm;
    float levelPercent;
    float currentA;
    float vibrationRmsG;
    float xG;
    float yG;
    float zG;
    bool tempValid;
    bool levelValid;
    bool vibrationReady;
    bool relayOn;
  };

  void begin();
  void update();

  Snapshot getSnapshot();

  float getLevelPercent();
  void setLevelThresholds(float fullPercent, float lowPercent);
  float getFullLevelPercent();
  float getLowLevelPercent();
  void setRelayLatchMode(RelayLatchMode mode);
  RelayLatchMode getRelayLatchMode();
}

#endif
