#include <Arduino.h>
#include "solar_level.h"

namespace solar_level {
static uint8_t solarPin = 255;
static float scaleFactor = 4.0f;
static float voltage = 0;
static uint8_t percentage = 0;
static unsigned long lastSampleTime = 0;
static uint32_t sampleTotal = 0;
static uint8_t sampleCount = 0;

static const uint16_t SAMPLE_INTERVAL_MS = 100;
static const uint8_t SAMPLES_PER_READING = 10;

void begin(uint8_t pin, float scale)
{
  solarPin = pin;
  scaleFactor = scale;
  analogReadResolution(12);
  analogSetPinAttenuation(solarPin, ADC_11db);
}

void update()
{
  const unsigned long now = millis();
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS)
    return;

  lastSampleTime = now;
  sampleTotal += analogRead(solarPin);
  sampleCount++;

  if (sampleCount < SAMPLES_PER_READING)
    return;

  const float averageAdc = sampleTotal / (float)sampleCount;
  const float adcVoltage = (averageAdc / 4095.0f) * 3.3f;
  voltage = adcVoltage * scaleFactor;

  float pct = (voltage / 12.0f) * 100.0f;
  if (pct < 0)
    pct = 0;
  if (pct > 100)
    pct = 100;
  percentage = (uint8_t)pct;

  sampleTotal = 0;
  sampleCount = 0;
}

float getVoltage()
{
  return voltage;
}

uint8_t getPercentage()
{
  return percentage;
}
}
