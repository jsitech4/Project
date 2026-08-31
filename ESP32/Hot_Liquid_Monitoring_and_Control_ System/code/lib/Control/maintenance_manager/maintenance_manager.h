#ifndef MAINTENANCE_MANAGER_H
#define MAINTENANCE_MANAGER_H

#include <Arduino.h>

namespace maintenance_manager
{
  enum State
  {
    STATE_NORMAL = 0,
    STATE_WARNING = 1,
    STATE_CRITICAL = 2,
    STATE_FAULT = 3
  };

  struct Snapshot
  {
    unsigned long uptimeMs;
    float temperatureC;
    float currentA;
    float vibrationRmsG;
    float xG;
    float yG;
    float zG;
    float riskScore;
    float healthScore;
    float forecastMinutes;
    float tempRatePerMin;
    float currentRatePerMin;
    float vibrationRatePerMin;
    bool tempValid;
    bool vibrationReady;
    bool relayOn;
    bool relayFault;
    State state;
  };

  void begin();
  void update();

  Snapshot getSnapshot();

  bool isNormal();
  bool isWarning();
  bool isCritical();
  bool isFault();

  float getRiskScore();
  float getHealthScore();
  float getForecastMinutes();

  String getLevelText();
  String getWorstMetric();
  String getRecommendation();

  void clearFault();

  void setCurrentLimits(float warningA, float faultA);
  void setTemperatureLimits(float warningC, float faultC);
  void setVibrationLimits(float warningG, float faultG);

  float getCurrentWarningLimit();
  float getCurrentFaultLimit();
  float getTemperatureWarningLimit();
  float getTemperatureFaultLimit();
  float getVibrationWarningLimit();
  float getVibrationFaultLimit();
}

#endif
