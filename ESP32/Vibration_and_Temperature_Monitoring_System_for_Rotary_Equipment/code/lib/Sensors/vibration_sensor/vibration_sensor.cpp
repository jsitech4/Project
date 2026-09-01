#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <math.h>

#include "Pins.h"
#include "vibration_sensor.h"

namespace vibration_sensor
{
  // =========================================================
  // SENSOR 1
  // =========================================================

  static Adafruit_ADXL345_Unified adxl1 =
      Adafruit_ADXL345_Unified(12345);

  static bool ready1 = false;

  static float x1 = 0.0f;
  static float y1 = 0.0f;
  static float z1 = 0.0f;

  static float baseMagnitude1 = 9.81f;
  static float vibrationRms1 = 0.0f;

  static bool sampling1 = false;
  static uint8_t samplesTaken1 = 0;
  static float sumSq1 = 0.0f;

  static unsigned long lastSample1 = 0;
  static unsigned long lastCycle1 = 0;

  // =========================================================
  // SENSOR 2
  // =========================================================

  static Adafruit_ADXL345_Unified adxl2 =
      Adafruit_ADXL345_Unified(67890);

  static bool ready2 = false;

  static float x2 = 0.0f;
  static float y2 = 0.0f;
  static float z2 = 0.0f;

  static float baseMagnitude2 = 9.81f;
  static float vibrationRms2 = 0.0f;

  static bool sampling2 = false;
  static uint8_t samplesTaken2 = 0;
  static float sumSq2 = 0.0f;

  static unsigned long lastSample2 = 0;
  static unsigned long lastCycle2 = 0;

  // =========================================================
  // CONFIGURATION
  // =========================================================

  static const uint8_t samplesTarget = 20;
  static const unsigned long samplePeriodMs = 2;
  static const unsigned long cycleIntervalMs = 100;

  // =========================================================
  // SENSOR 1 INTERNAL FUNCTIONS
  // =========================================================

  static float readSensor1Magnitude()
  {
    sensors_event_t event;

    adxl1.getEvent(&event);

    x1 = event.acceleration.x / 9.80665f;
    y1 = event.acceleration.y / 9.80665f;
    z1 = event.acceleration.z / 9.80665f;

    return sqrt(
        event.acceleration.x * event.acceleration.x +
        event.acceleration.y * event.acceleration.y +
        event.acceleration.z * event.acceleration.z);
  }

  static void startSampling1()
  {
    sampling1 = true;
    samplesTaken1 = 0;
    sumSq1 = 0.0f;
    lastSample1 = millis();
  }

  static void finishSampling1()
  {
    if (samplesTaken1 > 0)
    {
      vibrationRms1 =
          sqrt(sumSq1 / samplesTaken1);
    }

    sampling1 = false;
    lastCycle1 = millis();
  }

  // =========================================================
  // SENSOR 2 INTERNAL FUNCTIONS
  // =========================================================

  static float readSensor2Magnitude()
  {
    sensors_event_t event;

    adxl2.getEvent(&event);

    x2 = event.acceleration.x / 9.80665f;
    y2 = event.acceleration.y / 9.80665f;
    z2 = event.acceleration.z / 9.80665f;

    return sqrt(
        event.acceleration.x * event.acceleration.x +
        event.acceleration.y * event.acceleration.y +
        event.acceleration.z * event.acceleration.z);
  }

  static void startSampling2()
  {
    sampling2 = true;
    samplesTaken2 = 0;
    sumSq2 = 0.0f;
    lastSample2 = millis();
  }

  static void finishSampling2()
  {
    if (samplesTaken2 > 0)
    {
      vibrationRms2 =
          sqrt(sumSq2 / samplesTaken2);
    }

    sampling2 = false;
    lastCycle2 = millis();
  }

  // =========================================================
  // BEGIN
  // =========================================================

  void begin()
  {
    Wire.begin(
        Pins::I2C_SDA,
        Pins::I2C_SCL);

    // -----------------------------------------------------
    // SENSOR 1
    // ADXL345 address = 0x53
    // -----------------------------------------------------

    ready1 = adxl1.begin(0x53);

    if (ready1)
    {
      adxl1.setRange(ADXL345_RANGE_16_G);

      recalibrateSensor1();

      lastCycle1 = 0;

      startSampling1();
    }

    // -----------------------------------------------------
    // SENSOR 2
    // ADXL345 address = 0x1D
    // -----------------------------------------------------

    ready2 = adxl2.begin(0x1D);

    if (ready2)
    {
      adxl2.setRange(ADXL345_RANGE_16_G);

      recalibrateSensor2();

      lastCycle2 = 0;

      startSampling2();
    }
  }

  // =========================================================
  // UPDATE
  // =========================================================

  void update()
  {
    unsigned long now = millis();

    // =====================================================
    // SENSOR 1
    // =====================================================

    if (ready1)
    {
      if (!sampling1 &&
          now - lastCycle1 >= cycleIntervalMs)
      {
        startSampling1();
      }

      if (sampling1 &&
          now - lastSample1 >= samplePeriodMs)
      {
        lastSample1 = now;

        float mag =
            readSensor1Magnitude();

        float deltaG =
            (mag - baseMagnitude1) / 9.80665f;

        sumSq1 += deltaG * deltaG;

        samplesTaken1++;

        if (samplesTaken1 >= samplesTarget)
        {
          finishSampling1();
        }
      }
    }

    // =====================================================
    // SENSOR 2
    // =====================================================

    if (ready2)
    {
      if (!sampling2 &&
          now - lastCycle2 >= cycleIntervalMs)
      {
        startSampling2();
      }

      if (sampling2 &&
          now - lastSample2 >= samplePeriodMs)
      {
        lastSample2 = now;

        float mag =
            readSensor2Magnitude();

        float deltaG =
            (mag - baseMagnitude2) / 9.80665f;

        sumSq2 += deltaG * deltaG;

        samplesTaken2++;

        if (samplesTaken2 >= samplesTarget)
        {
          finishSampling2();
        }
      }
    }
  }

  // =========================================================
  // SENSOR 1 GETTERS
  // =========================================================

  float getSensor1X()
  {
    return x1;
  }

  float getSensor1Y()
  {
    return y1;
  }

  float getSensor1Z()
  {
    return z1;
  }

  float getSensor1VibrationRMS()
  {
    return vibrationRms1;
  }

  float getSensor1BaseMagnitude()
  {
    return baseMagnitude1;
  }

  bool isSensor1Ready()
  {
    return ready1;
  }

  // =========================================================
  // SENSOR 2 GETTERS
  // =========================================================

  float getSensor2X()
  {
    return x2;
  }

  float getSensor2Y()
  {
    return y2;
  }

  float getSensor2Z()
  {
    return z2;
  }

  float getSensor2VibrationRMS()
  {
    return vibrationRms2;
  }

  float getSensor2BaseMagnitude()
  {
    return baseMagnitude2;
  }

  bool isSensor2Ready()
  {
    return ready2;
  }

  // =========================================================
  // SENSOR 1 CALIBRATION
  // =========================================================

  void recalibrateSensor1()
  {
    if (!ready1)
      return;

    float sum = 0.0f;

    const uint8_t count = 20;

    for (uint8_t i = 0; i < count; i++)
    {
      sum += readSensor1Magnitude();

      delay(2);
    }

    baseMagnitude1 =
        sum / count;
  }

  // =========================================================
  // SENSOR 2 CALIBRATION
  // =========================================================

  void recalibrateSensor2()
  {
    if (!ready2)
      return;

    float sum = 0.0f;

    const uint8_t count = 20;

    for (uint8_t i = 0; i < count; i++)
    {
      sum += readSensor2Magnitude();

      delay(2);
    }

    baseMagnitude2 =
        sum / count;
  }

  // =========================================================
  // OVERALL STATUS
  // =========================================================

  bool isReady()
  {
    return ready1 && ready2;
  }
}
