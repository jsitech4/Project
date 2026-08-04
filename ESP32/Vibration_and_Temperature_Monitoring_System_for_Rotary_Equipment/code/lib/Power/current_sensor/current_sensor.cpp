#include <Arduino.h>
#include <math.h>
#include "Pins.h"
#include "current_sensor.h"

namespace current_sensor
{
  static const float ADC_REF_V = 3.3f;
  static const int ADC_MAX = 4095;
  static const uint16_t SAMPLE_TARGET = 800;
  static const unsigned long SAMPLE_PERIOD_US = 250;
  static const unsigned long CYCLE_INTERVAL_MS = 500;

  static float sensorCalibration = 30.0f;
  static float currentA = 0.0f;
  static float voltageRms = 0.0f;
  static int rawAdc = 0;
  static bool freshReading = false;

  static bool sampling = false;
  static uint16_t samplesTaken = 0;
  static double sum = 0.0;
  static double sumSq = 0.0;
  static unsigned long lastSampleUs = 0;
  static unsigned long lastCycleMs = 0;

  static void startSampling()
  {
    sampling = true;
    samplesTaken = 0;
    sum = 0.0;
    sumSq = 0.0;
    lastSampleUs = micros();
    freshReading = false;
  }

  static void finishSampling()
  {
    if (samplesTaken == 0)
    {
      sampling = false;
      return;
    }

    double mean = sum / samplesTaken;
    double meanSq = sumSq / samplesTaken;
    double variance = meanSq - (mean * mean);

    if (variance < 0.0)
      variance = 0.0;

    voltageRms = sqrt(variance);
    currentA = voltageRms * sensorCalibration;
    sampling = false;
    freshReading = true;
    lastCycleMs = millis();
  }

  void begin()
  {
    analogReadResolution(12);
    analogSetPinAttenuation(Pins::ZMCT103C_ADC, ADC_11db);
    lastCycleMs = 0;
    startSampling();
  }

  void update()
  {
    unsigned long nowMs = millis();

    if (!sampling && nowMs - lastCycleMs >= CYCLE_INTERVAL_MS)
    {
      startSampling();
    }

    if (!sampling)
      return;

    unsigned long nowUs = micros();
    uint8_t burstCount = 0;

    while ((long)(nowUs - lastSampleUs) >= (long)SAMPLE_PERIOD_US && samplesTaken < SAMPLE_TARGET && burstCount < 8)
    {
      lastSampleUs += SAMPLE_PERIOD_US;
      rawAdc = analogRead(Pins::ZMCT103C_ADC);
      float v = (rawAdc * ADC_REF_V) / ADC_MAX;
      sum += v;
      sumSq += v * v;
      samplesTaken++;
      burstCount++;
      nowUs = micros();
      yield();
    }

    if (samplesTaken >= SAMPLE_TARGET)
    {
      finishSampling();
    }
  }

  float getCurrentA()
  {
    return currentA;
  }

  float getVoltageRMS()
  {
    return voltageRms;
  }

  int getRawADC()
  {
    return rawAdc;
  }

  bool hasFreshReading()
  {
    bool value = freshReading;
    freshReading = false;
    return value;
  }

  void setCalibration(float calibration)
  {
    if (calibration > 0.0f)
    {
      sensorCalibration = calibration;
    }
  }

  float getCalibration()
  {
    return sensorCalibration;
  }
}
