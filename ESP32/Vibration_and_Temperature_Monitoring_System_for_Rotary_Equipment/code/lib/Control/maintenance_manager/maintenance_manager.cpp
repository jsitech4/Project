#include <Arduino.h>
#include <math.h>
#include "maintenance_manager.h"
#include "temp_sensor/temp_sensor.h"
#include "vibration_sensor/vibration_sensor.h"
#include "load_relay/load_relay.h"
#include "buzzer/buzzer.h"
#include "error_handling/error_handling.h"

namespace maintenance_manager
{
  static float tempWarnC = 60.0f;
  static float tempFaultC = 70.0f;
  static float vibrationWarnG = 1.2f;
  static float vibrationFaultG = 2.5f;

  static Snapshot snap;
  static State state = STATE_NORMAL;
  static bool faultLatched = false;
  static uint8_t faultCounter = 0;
  static uint8_t safeCounter = 0;

  static unsigned long lastUpdate = 0;
  static unsigned long lastTrendTime = 0;
  static unsigned long lastWarningBeep = 0;

  static float prevTemp = NAN;

  static float prevVibration1 = NAN;
  static float prevVibration2 = NAN;

  static String worstMetric = "none";
  static String recommendation = "System normal. Continue monitoring.";

  static float limitFloat(float value, float low, float high)
  {
    if (value < low)
      return low;
    if (value > high)
      return high;
    return value;
  }

  static float severity(float value, float warning, float fault)
  {
    if (fault <= warning)
      return 0.0f;

    if (value <= 0.0f)
      return 0.0f;

    if (value < warning)
    {
      return limitFloat((value / warning) * 35.0f, 0.0f, 35.0f);
    }

    if (value < fault)
    {
      return 40.0f + ((value - warning) / (fault - warning)) * 45.0f;
    }

    return limitFloat(90.0f + ((value - fault) / fault) * 10.0f, 90.0f, 100.0f);
  }

  static float minutesToLimit(float value, float ratePerMin, float limit)
  {
    if (ratePerMin <= 0.001f)
      return -1.0f;

    if (value >= limit)
      return 0.0f;

    float minutes = (limit - value) / ratePerMin;

    if (minutes < 0.0f || minutes > 9999.0f)
      return -1.0f;

    return minutes;
  }

  static void updateTrend(
      float tempC,
      bool tempValid,
      float vibration1G,
      float vibration2G)
  {
    unsigned long now = millis();

    if (lastTrendTime == 0)
    {
      lastTrendTime = now;

      prevTemp = tempC;
      prevVibration1 = vibration1G;
      prevVibration2 = vibration2G;

      snap.tempRatePerMin = 0.0f;
      snap.vibration1RatePerMin = 0.0f;
      snap.vibration2RatePerMin = 0.0f;

      return;
    }

    float dtMin =
        (now - lastTrendTime) / 60000.0f;

    if (dtMin < 0.2f)
      return;

    if (tempValid && !isnan(prevTemp))
      snap.tempRatePerMin =
          (tempC - prevTemp) / dtMin;
    else
      snap.tempRatePerMin = 0.0f;

    if (!isnan(prevVibration1))
      snap.vibration1RatePerMin =
          (vibration1G - prevVibration1) / dtMin;
    else
      snap.vibration1RatePerMin = 0.0f;

    if (!isnan(prevVibration2))
      snap.vibration2RatePerMin =
          (vibration2G - prevVibration2) / dtMin;
    else
      snap.vibration2RatePerMin = 0.0f;

    lastTrendTime = now;

    prevTemp = tempC;
    prevVibration1 = vibration1G;
    prevVibration2 = vibration2G;
  }

  static void updateForecast(
      float tempC,
      bool tempValid,
      float vibration1G,
      float vibration2G)
  {
    float forecast = -1.0f;

    float t1 =
        tempValid
            ? minutesToLimit(
                  tempC,
                  snap.tempRatePerMin,
                  tempFaultC)
            : -1.0f;

    float t2 =
        minutesToLimit(
            vibration1G,
            snap.vibration1RatePerMin,
            vibrationFaultG);

    float t3 =
        minutesToLimit(
            vibration2G,
            snap.vibration2RatePerMin,
            vibrationFaultG);

    if (t1 >= 0.0f)
      forecast = t1;

    if (t2 >= 0.0f &&
        (forecast < 0.0f || t2 < forecast))
      forecast = t2;

    if (t3 >= 0.0f &&
        (forecast < 0.0f || t3 < forecast))
      forecast = t3;

    snap.forecastMinutes = forecast;
  }

  static void updateRecommendation(
      float tempSeverity,
      float vibration1Severity,
      float vibration2Severity)
  {
    if (tempSeverity >= vibration1Severity &&
        tempSeverity >= vibration2Severity)
    {
      worstMetric = "temperature";

      if (tempSeverity >= 90.0f)
        recommendation =
            "Over-temperature fault. Stop motor and inspect cooling, load, and bearing friction.";
      else if (tempSeverity >= 60.0f)
        recommendation =
            "Temperature rising. Check ventilation, load level, lubrication, and motor casing heat.";
      else
        recommendation =
            "Temperature normal. Continue monitoring.";
    }
    else if (vibration1Severity >= vibration2Severity)
    {
      worstMetric = "vibration_sensor_1";

      if (vibration1Severity >= 90.0f)
        recommendation =
            "High vibration fault on Sensor 1. Inspect bearing, alignment, mounting, shaft balance, and coupling.";
      else if (vibration1Severity >= 60.0f)
        recommendation =
            "Vibration Sensor 1 is increasing. Plan bearing/alignment inspection before failure.";
      else
        recommendation =
            "Vibration Sensor 1 normal. Continue monitoring.";
    }
    else
    {
      worstMetric = "vibration_sensor_2";

      if (vibration2Severity >= 90.0f)
        recommendation =
            "High vibration fault on Sensor 2. Inspect bearing, alignment, mounting, shaft balance, and coupling.";
      else if (vibration2Severity >= 60.0f)
        recommendation =
            "Vibration Sensor 2 is increasing. Plan bearing/alignment inspection before failure.";
      else
        recommendation =
            "Vibration Sensor 2 normal. Continue monitoring.";
    }

    if (snap.forecastMinutes >= 0.0f &&
        snap.forecastMinutes <= 60.0f &&
        !faultLatched)
    {
      recommendation +=
          " Predicted fault window is under one hour.";
    }
  }

  void begin()
  {
    memset(&snap, 0, sizeof(snap));
    snap.healthScore = 100.0f;
    snap.forecastMinutes = -1.0f;
    state = STATE_NORMAL;
    faultLatched = false;
    faultCounter = 0;
    safeCounter = 0;
    lastUpdate = 0;
    lastTrendTime = 0;
    lastWarningBeep = 0;
    worstMetric = "none";
    recommendation = "System normal. Continue monitoring.";
  }

  void update()
  {
    unsigned long now = millis();

    if (now - lastUpdate < 1000)
      return;

    lastUpdate = now;

    float tempC = temp_sensor::getTemperatureC();
    bool tempValid = temp_sensor::isValid();

    float vibration1G =
        vibration_sensor::getSensor1VibrationRMS();

    float vibration2G =
        vibration_sensor::getSensor2VibrationRMS();

    updateTrend(
        tempC,
        tempValid,
        vibration1G,
        vibration2G);
    updateForecast(
        tempC,
        tempValid,
        vibration1G,
        vibration2G);

    float tempSeverity = tempValid ? severity(tempC, tempWarnC, tempFaultC) : 0.0f;

    float vibration1Severity =
        vibration_sensor::isSensor1Ready()
            ? severity(
                  vibration1G,
                  vibrationWarnG,
                  vibrationFaultG)
            : 0.0f;

    float vibration2Severity =
        vibration_sensor::isSensor2Ready()
            ? severity(
                  vibration2G,
                  vibrationWarnG,
                  vibrationFaultG)
            : 0.0f;

    float weightedRisk =
        (tempSeverity * 0.30f) +
        (vibration1Severity * 0.35f) +
        (vibration2Severity * 0.35f);

    float maxRisk =
        max(
            tempSeverity,
            max(vibration1Severity, vibration2Severity));

    float trendBoost = 0.0f;

    if (snap.forecastMinutes >= 0.0f)
    {
      if (snap.forecastMinutes < 10.0f)
        trendBoost = 15.0f;
      else if (snap.forecastMinutes < 60.0f)
        trendBoost = 10.0f;
      else if (snap.forecastMinutes < 180.0f)
        trendBoost = 5.0f;
    }

    snap.riskScore = limitFloat((weightedRisk * 0.45f) + (maxRisk * 0.55f) + trendBoost, 0.0f, 100.0f);
    snap.healthScore = limitFloat(100.0f - snap.riskScore, 0.0f, 100.0f);

    bool hardFault = false;

    if (tempValid && tempC >= tempFaultC)
      hardFault = true;

    if (vibration_sensor::isSensor1Ready() &&
        vibration1G >= vibrationFaultG)
    {
      hardFault = true;
    }

    if (vibration_sensor::isSensor2Ready() &&
        vibration2G >= vibrationFaultG)
    {
      hardFault = true;
    }

    if (hardFault)
    {
      if (faultCounter < 5)
        faultCounter++;
      safeCounter = 0;
    }
    else
    {
      faultCounter = 0;

      bool safe = true;

      if (tempValid && tempC >= tempWarnC)
        safe = false;

      if (vibration_sensor::isSensor1Ready() &&
          vibration1G >= vibrationWarnG)
      {
        safe = false;
      }

      if (vibration_sensor::isSensor2Ready() &&
          vibration2G >= vibrationWarnG)
      {
        safe = false;
      }

      if (safe && safeCounter < 10)
        safeCounter++;
      else if (!safe)
        safeCounter = 0;
    }

    if (faultCounter >= 2)
      faultLatched = true;

    if (faultLatched)
      state = STATE_FAULT;
    else if (snap.riskScore >= 75.0f)
      state = STATE_CRITICAL;
    else if (snap.riskScore >= 50.0f)
      state = STATE_WARNING;
    else
      state = STATE_NORMAL;

    snap.uptimeMs = now;
    snap.temperatureC = tempC;

    snap.vibration1RmsG = vibration1G;
    snap.vibration2RmsG = vibration2G;

    snap.vibration1XG =
        vibration_sensor::getSensor1X();

    snap.vibration1YG =
        vibration_sensor::getSensor1Y();

    snap.vibration1ZG =
        vibration_sensor::getSensor1Z();

    snap.vibration2XG =
        vibration_sensor::getSensor2X();

    snap.vibration2YG =
        vibration_sensor::getSensor2Y();

    snap.vibration2ZG =
        vibration_sensor::getSensor2Z();

    snap.tempValid = tempValid;

    snap.vibration1Ready =
        vibration_sensor::isSensor1Ready();

    snap.vibration2Ready =
        vibration_sensor::isSensor2Ready();

    snap.relayOn = load_relay::isOn();
    snap.relayFault = load_relay::isFault();
    snap.state = state;

    updateRecommendation(
        tempSeverity,
        vibration1Severity,
        vibration2Severity);

    if (faultLatched)
    {
      load_relay::trip();
      buzzer::startAlarm();
      error_handling::setCodeError(true);
    }
    else
    {
      error_handling::setCodeError(false);
      load_relay::update();

      if (state == STATE_CRITICAL || state == STATE_WARNING)
      {
        unsigned long beepInterval = state == STATE_CRITICAL ? 2500UL : 7000UL;

        if (now - lastWarningBeep >= beepInterval)
        {
          lastWarningBeep = now;
          buzzer::beep(state == STATE_CRITICAL ? 120 : 60);
        }
      }
      else
      {
        buzzer::stopAlarm();
      }
    }
  }

  Snapshot getSnapshot()
  {
    return snap;
  }

  bool isNormal()
  {
    return state == STATE_NORMAL;
  }

  bool isWarning()
  {
    return state == STATE_WARNING;
  }

  bool isCritical()
  {
    return state == STATE_CRITICAL;
  }

  bool isFault()
  {
    return state == STATE_FAULT || faultLatched;
  }

  float getRiskScore()
  {
    return snap.riskScore;
  }

  float getHealthScore()
  {
    return snap.healthScore;
  }

  float getForecastMinutes()
  {
    return snap.forecastMinutes;
  }

  String getLevelText()
  {
    if (state == STATE_FAULT)
      return "FAULT";
    if (state == STATE_CRITICAL)
      return "CRITICAL";
    if (state == STATE_WARNING)
      return "WARNING";
    return "NORMAL";
  }

  String getWorstMetric()
  {
    return worstMetric;
  }

  String getRecommendation()
  {
    return recommendation;
  }

  void clearFault()
  {
    faultCounter = 0;
    safeCounter = 0;
    faultLatched = false;
    state = STATE_NORMAL;
    load_relay::clearFault();
    buzzer::stopAlarm();
    error_handling::setCodeError(false);
  }

  void setTemperatureLimits(float warningC, float faultC)
  {
    if (faultC > warningC)
    {
      tempWarnC = warningC;
      tempFaultC = faultC;
    }
  }

  void setVibrationLimits(float warningG, float faultG)
  {
    if (warningG > 0.0f && faultG > warningG)
    {
      vibrationWarnG = warningG;
      vibrationFaultG = faultG;
    }
  }

  float getTemperatureWarningLimit()
  {
    return tempWarnC;
  }

  float getTemperatureFaultLimit()
  {
    return tempFaultC;
  }

  float getVibrationWarningLimit()
  {
    return vibrationWarnG;
  }

  float getVibrationFaultLimit()
  {
    return vibrationFaultG;
  }
}
