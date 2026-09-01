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

    float vibration1RmsG;
    float vibration2RmsG;

    float vibration1XG;
    float vibration1YG;
    float vibration1ZG;

    float vibration2XG;
    float vibration2YG;
    float vibration2ZG;

    float riskScore;
    float healthScore;
    float forecastMinutes;

    float tempRatePerMin;
    float vibration1RatePerMin;
    float vibration2RatePerMin;

    bool tempValid;
    bool vibration1Ready;
    bool vibration2Ready;

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

  void setTemperatureLimits(float warningC, float faultC);

  void setVibrationLimits(float warningG, float faultG);

  float getTemperatureWarningLimit();
  float getTemperatureFaultLimit();

  float getVibrationWarningLimit();
  float getVibrationFaultLimit();
}

#endif
