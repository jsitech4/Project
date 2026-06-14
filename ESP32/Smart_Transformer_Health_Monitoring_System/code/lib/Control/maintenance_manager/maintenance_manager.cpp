#include <Arduino.h>
#include <math.h>
#include "maintenance_manager.h"
#include "current_sensor/current_sensor.h"
#include "voltage_sensor/voltage_sensor.h"
#include "temp_sensor/temp_sensor.h"
#include "vibration_sensor/vibration_sensor.h"
#include "load_relay/load_relay.h"
#include "buzzer/buzzer.h"
#include "error_handling/error_handling.h"

namespace maintenance_manager
{
  static float currentWarnA = 0.07f;
  static float currentFaultA = 0.14f;
  static float tempWarnC = 60.0f;
  static float tempFaultC = 70.0f;
  static float vibrationWarnG = 0.2f;
  static float vibrationFaultG = 0.6f;
  static float voltageLowWarnV = 180.0f;
  static float voltageLowFaultV = 170.0f;
  static float voltageHighWarnV = 235.0f;
  static float voltageHighFaultV = 240.0f;

  static Snapshot snap;
  static State state = STATE_NORMAL;
  static bool faultLatched = false;
  static uint8_t faultCounter = 0;
  static uint8_t safeCounter = 0;

  static unsigned long lastUpdate = 0;
  static unsigned long lastTrendTime = 0;
  static unsigned long lastWarningBeep = 0;

  static float prevTemp = NAN;
  static float prevCurrent = NAN;
  static float prevVoltage = NAN;
  static float prevVibration = NAN;
  static String worstMetric = "none";
  static String recommendation = "Transformer condition normal. Continue monitoring.";

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
      return limitFloat((value / warning) * 35.0f, 0.0f, 35.0f);

    if (value < fault)
      return 40.0f + ((value - warning) / (fault - warning)) * 45.0f;

    return limitFloat(90.0f + ((value - fault) / fault) * 10.0f, 90.0f, 100.0f);
  }

  static float voltageSeverity(float voltage)
  {
    if (voltage < 10.0f)
      return 0.0f;

    if (voltage < voltageLowFaultV)
      return limitFloat(90.0f + ((voltageLowFaultV - voltage) / voltageLowFaultV) * 10.0f, 90.0f, 100.0f);

    if (voltage < voltageLowWarnV)
      return 40.0f + ((voltageLowWarnV - voltage) / (voltageLowWarnV - voltageLowFaultV)) * 45.0f;

    if (voltage <= voltageHighWarnV)
      return 0.0f;

    if (voltage < voltageHighFaultV)
      return 40.0f + ((voltage - voltageHighWarnV) / (voltageHighFaultV - voltageHighWarnV)) * 45.0f;

    return limitFloat(90.0f + ((voltage - voltageHighFaultV) / voltageHighFaultV) * 10.0f, 90.0f, 100.0f);
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

  static float minutesToLowLimit(float value, float ratePerMin, float limit)
  {
    if (ratePerMin >= -0.001f)
      return -1.0f;

    if (value <= limit)
      return 0.0f;

    float minutes = (limit - value) / ratePerMin;

    if (minutes < 0.0f || minutes > 9999.0f)
      return -1.0f;

    return minutes;
  }

  static void updateTrend(float tempC, bool tempValid, float currentA, float voltageV, float vibrationG)
  {
    unsigned long now = millis();

    if (lastTrendTime == 0)
    {
      lastTrendTime = now;
      prevTemp = tempC;
      prevCurrent = currentA;
      prevVoltage = voltageV;
      prevVibration = vibrationG;
      snap.tempRatePerMin = 0.0f;
      snap.currentRatePerMin = 0.0f;
      snap.voltageRatePerMin = 0.0f;
      snap.vibrationRatePerMin = 0.0f;
      return;
    }

    float dtMin = (now - lastTrendTime) / 60000.0f;

    if (dtMin < 0.2f)
      return;

    if (tempValid && !isnan(prevTemp))
      snap.tempRatePerMin = (tempC - prevTemp) / dtMin;
    else
      snap.tempRatePerMin = 0.0f;

    if (!isnan(prevCurrent))
      snap.currentRatePerMin = (currentA - prevCurrent) / dtMin;
    else
      snap.currentRatePerMin = 0.0f;

    if (!isnan(prevVoltage))
      snap.voltageRatePerMin = (voltageV - prevVoltage) / dtMin;
    else
      snap.voltageRatePerMin = 0.0f;

    if (!isnan(prevVibration))
      snap.vibrationRatePerMin = (vibrationG - prevVibration) / dtMin;
    else
      snap.vibrationRatePerMin = 0.0f;

    lastTrendTime = now;
    prevTemp = tempC;
    prevCurrent = currentA;
    prevVoltage = voltageV;
    prevVibration = vibrationG;
  }

  static void updateForecast(float tempC, bool tempValid, float currentA, float voltageV, float vibrationG)
  {
    float forecast = -1.0f;

    float t1 = tempValid ? minutesToLimit(tempC, snap.tempRatePerMin, tempFaultC) : -1.0f;
    float t2 = minutesToLimit(currentA, snap.currentRatePerMin, currentFaultA);
    float t3 = minutesToLimit(vibrationG, snap.vibrationRatePerMin, vibrationFaultG);
    float t4 = minutesToLimit(voltageV, snap.voltageRatePerMin, voltageHighFaultV);
    float t5 = voltageV > 10.0f ? minutesToLowLimit(voltageV, snap.voltageRatePerMin, voltageLowFaultV) : -1.0f;

    if (t1 >= 0.0f)
      forecast = t1;
    if (t2 >= 0.0f && (forecast < 0.0f || t2 < forecast))
      forecast = t2;
    if (t3 >= 0.0f && (forecast < 0.0f || t3 < forecast))
      forecast = t3;
    if (t4 >= 0.0f && (forecast < 0.0f || t4 < forecast))
      forecast = t4;
    if (t5 >= 0.0f && (forecast < 0.0f || t5 < forecast))
      forecast = t5;

    snap.forecastMinutes = forecast;
  }

  static void updateRecommendation(float tempSeverity, float currentSeverity, float voltageSeverityValue, float vibrationSeverity)
  {
    float maxSeverity = tempSeverity;
    worstMetric = "temperature";

    if (currentSeverity > maxSeverity)
    {
      maxSeverity = currentSeverity;
      worstMetric = "current";
    }

    if (voltageSeverityValue > maxSeverity)
    {
      maxSeverity = voltageSeverityValue;
      worstMetric = "voltage";
    }

    if (vibrationSeverity > maxSeverity)
    {
      maxSeverity = vibrationSeverity;
      worstMetric = "vibration";
    }

    if (worstMetric == "temperature")
    {
      if (tempSeverity >= 90.0f)
        recommendation = "Over-temperature fault. Isolate transformer load and inspect cooling, winding heat, terminal tightness, and loading condition.";
      else if (tempSeverity >= 60.0f)
        recommendation = "Temperature rising. Check transformer ventilation, ambient heat, load level, and winding/case temperature trend.";
      else
        recommendation = "Transformer temperature normal. Continue monitoring.";
    }
    else if (worstMetric == "current")
    {
      if (currentSeverity >= 90.0f)
        recommendation = "Over-current fault. Check transformer load, downstream short circuit, overload condition, and protection device status.";
      else if (currentSeverity >= 60.0f)
        recommendation = "Current is high. Review transformer loading, load balance, and downstream equipment demand.";
      else
        recommendation = "Transformer current normal. Continue monitoring.";
    }
    else if (worstMetric == "voltage")
    {
      if (voltageSeverityValue >= 90.0f)
        recommendation = "Abnormal transformer voltage fault. Isolate the system and inspect input supply, output terminals, tap setting, and voltage sensing circuit.";
      else if (voltageSeverityValue >= 60.0f)
        recommendation = "Transformer voltage is drifting. Check supply stability, load condition, tap selection, and sensor calibration.";
      else
        recommendation = "Transformer voltage normal. Continue monitoring.";
    }
    else
    {
      if (vibrationSeverity >= 90.0f)
        recommendation = "High vibration fault. Inspect transformer mounting, core hum, loose laminations, enclosure vibration, and nearby mechanical coupling.";
      else if (vibrationSeverity >= 60.0f)
        recommendation = "Vibration is increasing. Plan inspection for mounting looseness, abnormal transformer hum, and enclosure resonance.";
      else
        recommendation = "Transformer vibration normal. Continue monitoring.";
    }

    if (snap.forecastMinutes >= 0.0f && snap.forecastMinutes <= 60.0f && !faultLatched)
      recommendation += " Predicted fault window is under one hour.";
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
    recommendation = "Transformer condition normal. Continue monitoring.";
  }

  void update()
  {
    unsigned long now = millis();

    if (now - lastUpdate < 1000)
      return;

    lastUpdate = now;

    float tempC = temp_sensor::getTemperatureC();
    bool tempValid = temp_sensor::isValid();
    float currentA = current_sensor::getCurrentA();
    float voltageV = voltage_sensor::getVoltageRMS();
    float vibrationG = vibration_sensor::getVibrationRMS();

    updateTrend(tempC, tempValid, currentA, voltageV, vibrationG);
    updateForecast(tempC, tempValid, currentA, voltageV, vibrationG);

    float tempSeverity = tempValid ? severity(tempC, tempWarnC, tempFaultC) : 0.0f;
    float currentSeverity = severity(currentA, currentWarnA, currentFaultA);
    float voltageSeverityValue = voltageSeverity(voltageV);
    float vibrationSeverity = vibration_sensor::isReady() ? severity(vibrationG, vibrationWarnG, vibrationFaultG) : 0.0f;

    float weightedRisk = (tempSeverity * 0.25f) + (currentSeverity * 0.25f) + (voltageSeverityValue * 0.20f) + (vibrationSeverity * 0.30f);
    float maxRisk = max(max(tempSeverity, currentSeverity), max(voltageSeverityValue, vibrationSeverity));

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
    if (currentA >= currentFaultA)
      hardFault = true;
    if (voltageV > 10.0f && (voltageV <= voltageLowFaultV || voltageV >= voltageHighFaultV))
      hardFault = true;
    if (vibration_sensor::isReady() && vibrationG >= vibrationFaultG)
      hardFault = true;

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
      if (currentA >= currentWarnA)
        safe = false;
      if (voltageV > 10.0f && (voltageV <= voltageLowWarnV || voltageV >= voltageHighWarnV))
        safe = false;
      if (vibration_sensor::isReady() && vibrationG >= vibrationWarnG)
        safe = false;

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
    snap.currentA = currentA;
    snap.transformerVoltageV = voltageV;
    snap.voltageAdcRmsV = voltage_sensor::getAdcVoltageRMS();
    snap.vibrationRmsG = vibrationG;
    snap.xG = vibration_sensor::getX();
    snap.yG = vibration_sensor::getY();
    snap.zG = vibration_sensor::getZ();
    snap.tempValid = tempValid;
    snap.vibrationReady = vibration_sensor::isReady();
    snap.relayOn = load_relay::isOn();
    snap.relayFault = load_relay::isFault();
    snap.state = state;

    updateRecommendation(tempSeverity, currentSeverity, voltageSeverityValue, vibrationSeverity);

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

  void setCurrentLimits(float warningA, float faultA)
  {
    if (warningA > 0.0f && faultA > warningA)
    {
      currentWarnA = warningA;
      currentFaultA = faultA;
    }
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

  void setVoltageLimits(float lowWarningV, float lowFaultV, float highWarningV, float highFaultV)
  {
    if (lowFaultV > 0.0f && lowWarningV > lowFaultV && highWarningV > lowWarningV && highFaultV > highWarningV)
    {
      voltageLowWarnV = lowWarningV;
      voltageLowFaultV = lowFaultV;
      voltageHighWarnV = highWarningV;
      voltageHighFaultV = highFaultV;
    }
  }

  float getCurrentWarningLimit()
  {
    return currentWarnA;
  }

  float getCurrentFaultLimit()
  {
    return currentFaultA;
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

  float getVoltageLowWarningLimit()
  {
    return voltageLowWarnV;
  }

  float getVoltageLowFaultLimit()
  {
    return voltageLowFaultV;
  }

  float getVoltageHighWarningLimit()
  {
    return voltageHighWarnV;
  }

  float getVoltageHighFaultLimit()
  {
    return voltageHighFaultV;
  }
}
