#include <Arduino.h>
#include "battery_level.h"

namespace battery_level {
static uint8_t batteryPin = 255;
static float scaleFactor = 2.0f;
static float voltage = 0;
static uint8_t percentage = 0;
static unsigned long lastSampleTime = 0;
static uint32_t sampleTotal = 0;
static uint8_t sampleCount = 0;

static const uint16_t SAMPLE_INTERVAL_MS = 100;
static const uint8_t SAMPLES_PER_READING = 10;

void begin(uint8_t pin, float scale)
{
  batteryPin = pin;
  scaleFactor = scale;
  analogReadResolution(12);
  analogSetPinAttenuation(batteryPin, ADC_11db);
}

void update()
{
  const unsigned long now = millis();
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS)
    return;

  lastSampleTime = now;
  sampleTotal += analogRead(batteryPin);
  sampleCount++;

  if (sampleCount < SAMPLES_PER_READING)
    return;

  const float averageAdc = sampleTotal / (float)sampleCount;
  const float adcVoltage = (averageAdc / 4095.0f) * 3.3f;
  voltage = adcVoltage * scaleFactor;

  float pct = ((voltage - 3.2f) / (4.2f - 3.2f)) * 100.0f;
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
