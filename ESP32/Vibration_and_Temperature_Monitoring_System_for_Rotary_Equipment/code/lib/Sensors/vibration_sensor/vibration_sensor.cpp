#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <math.h>
#include "Pins.h"
#include "vibration_sensor.h"

namespace vibration_sensor
{
  static Adafruit_ADXL345_Unified adxl = Adafruit_ADXL345_Unified(12345);

  static bool ready = false;

  static float x = 0.0f;
  static float y = 0.0f;
  static float z = 0.0f;

  static float baseMagnitude = 9.81f;
  static float vibrationRms = 0.0f;

  static const uint8_t samplesTarget = 20;
  static const unsigned long samplePeriodMs = 2;
  static const unsigned long cycleIntervalMs = 100;

  static bool sampling = false;
  static uint8_t samplesTaken = 0;
  static float sumSq = 0.0f;
  static unsigned long lastSample = 0;
  static unsigned long lastCycle = 0;

  static float readMagnitudeAndUpdateAxis()
  {
    sensors_event_t event;
    adxl.getEvent(&event);

    x = event.acceleration.x / 9.80665f;
    y = event.acceleration.y / 9.80665f;
    z = event.acceleration.z / 9.80665f;

    return sqrt(
        event.acceleration.x * event.acceleration.x +
        event.acceleration.y * event.acceleration.y +
        event.acceleration.z * event.acceleration.z);
  }

  static void startSampling()
  {
    sampling = true;
    samplesTaken = 0;
    sumSq = 0.0f;
    lastSample = millis();
  }

  static void finishSampling()
  {
    if (samplesTaken > 0)
      vibrationRms = sqrt(sumSq / samplesTaken);

    sampling = false;
    lastCycle = millis();
  }

  void begin()
  {
    Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

    ready = adxl.begin(0x53);

    if (ready)
    {
      adxl.setRange(ADXL345_RANGE_16_G);
      recalibrateBaseline();
      lastCycle = 0;
      startSampling();
    }
  }

  void update()
  {
    if (!ready)
      return;

    unsigned long now = millis();

    if (!sampling && now - lastCycle >= cycleIntervalMs)
      startSampling();

    if (!sampling)
      return;

    if (now - lastSample < samplePeriodMs)
      return;

    lastSample = now;

    float mag = readMagnitudeAndUpdateAxis();
    float deltaG = (mag - baseMagnitude) / 9.80665f;

    sumSq += deltaG * deltaG;
    samplesTaken++;

    if (samplesTaken >= samplesTarget)
      finishSampling();
  }

  float getX()
  {
    return x;
  }

  float getY()
  {
    return y;
  }

  float getZ()
  {
    return z;
  }

  float getVibrationRMS()
  {
    return vibrationRms;
  }

  float getBaseMagnitude()
  {
    return baseMagnitude;
  }

  bool isReady()
  {
    return ready;
  }

  void recalibrateBaseline()
  {
    if (!ready)
      return;

    float sum = 0.0f;
    const uint8_t count = 20;

    for (uint8_t i = 0; i < count; i++)
    {
      sum += readMagnitudeAndUpdateAxis();
      delay(2);
    }

    baseMagnitude = sum / count;
  }
}
