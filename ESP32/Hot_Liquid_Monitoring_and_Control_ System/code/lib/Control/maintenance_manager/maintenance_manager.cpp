#include <Arduino.h>
#include <math.h>
#include "maintenance_manager.h"
#include "temp_sensor/temp_sensor.h"
#include "ultrasonic_sensor/ultrasonic_sensor.h"
#include "current_sensor/current_sensor.h"
#include "load_relay/load_relay.h"

namespace maintenance_manager
{
  static const float EMPTY_DISTANCE_CM = 40.0f;
  static const float FULL_DISTANCE_CM = 5.0f;
  static float fullLevelPercent = 90.0f;
  static float lowLevelPercent = 20.0f;
  static RelayLatchMode relayLatchMode = LATCH_OFF;
  static bool latchTriggered = false;
  static Snapshot snap;
  static unsigned long lastUpdate = 0;

  static float clamp(float value, float low, float high)
  {
    return value < low ? low : value > high ? high
                                            : value;
  }

  static float calculateLevel(float distanceCm)
  {
    if (distanceCm <= 0.0f)
      return NAN;

    return clamp((EMPTY_DISTANCE_CM - distanceCm) * 100.0f /
                     (EMPTY_DISTANCE_CM - FULL_DISTANCE_CM),
                 0.0f, 100.0f);
  }

  void begin()
  {
    memset(&snap, 0, sizeof(snap));
    snap.temperatureC = NAN;
    snap.distanceCm = NAN;
    snap.levelPercent = NAN;
    lastUpdate = 0;
  }

  void update()
  {
    unsigned long now = millis();
    if (now - lastUpdate < 1000)
      return;
    lastUpdate = now;

    snap.uptimeMs = now;
    snap.temperatureC = temp_sensor::getTemperatureC();
    snap.distanceCm = ultrasonic_sensor::getDistanceCm();
    snap.levelPercent = calculateLevel(snap.distanceCm);
    snap.currentA = current_sensor::getCurrentA();
    snap.vibrationRmsG = 0.0f;
    snap.xG = 0.0f;
    snap.yG = 0.0f;
    snap.zG = 0.0f;
    snap.tempValid = temp_sensor::isValid();
    snap.levelValid = snap.distanceCm > 0.0f;
    snap.vibrationReady = false;
    snap.relayOn = load_relay::isOn();

    bool thresholdReached = relayLatchMode == LATCH_AT_FULL
                                ? snap.levelValid && snap.levelPercent >= fullLevelPercent
                            : relayLatchMode == LATCH_AT_LOW
                                ? snap.levelValid && snap.levelPercent <= lowLevelPercent
                                : false;

    if (!thresholdReached)
      latchTriggered = false;
    else if (!latchTriggered)
    {
      load_relay::turnOn();
      latchTriggered = true;
    }
  }

  Snapshot getSnapshot() { return snap; }
  float getLevelPercent() { return snap.levelPercent; }

  void setLevelThresholds(float fullPercent, float lowPercent)
  {
    if (fullPercent > lowPercent && fullPercent <= 100.0f && lowPercent >= 0.0f)
    {
      fullLevelPercent = fullPercent;
      lowLevelPercent = lowPercent;
    }
  }

  float getFullLevelPercent() { return fullLevelPercent; }
  float getLowLevelPercent() { return lowLevelPercent; }

  void setRelayLatchMode(RelayLatchMode mode)
  {
    if (mode <= LATCH_AT_LOW)
    {
      relayLatchMode = mode;
      latchTriggered = false;
    }
  }

  RelayLatchMode getRelayLatchMode() { return relayLatchMode; }
}
