#include <Arduino.h>
#include <Wire.h>
#include "Pins.h"
#include "temp_sensor.h"

namespace temp_sensor
{
  static const uint8_t AHT20_ADDR = 0x38;

  static float temperatureC = NAN;
  static bool valid = false;

  static bool measurementPending = false;
  static unsigned long lastRequest = 0;

  static const unsigned long interval = 1000;
  static const unsigned long measurementDelay = 80;

  static bool sensorReady = false;

  static bool readAHT20(float &temp)
  {
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};

    Wire.beginTransmission(AHT20_ADDR);
    Wire.write(cmd, 3);
    if (Wire.endTransmission() != 0)
      return false;

    delay(measurementDelay);

    Wire.requestFrom(AHT20_ADDR, (uint8_t)6);
    if (Wire.available() < 6)
      return false;

    uint8_t data[6];
    for (int i = 0; i < 6; i++)
      data[i] = Wire.read();

    uint32_t raw = ((uint32_t)data[3] & 0x0F) << 16 |
                   ((uint32_t)data[4] << 8) |
                   data[5];

    temp = ((raw * 200.0f) / 1048576.0f) - 50.0f;

    return (temp > -40.0f && temp < 125.0f);
  }

  void begin()
  {
    Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);
    delay(50);

    Wire.beginTransmission(AHT20_ADDR);
    sensorReady = (Wire.endTransmission() == 0);

    temperatureC = NAN;
    valid = false;

    measurementPending = true;
    lastRequest = millis();
  }

  void update()
  {
    if (!sensorReady)
    {
      valid = false;
      return;
    }

    unsigned long now = millis();

    if (measurementPending && (now - lastRequest >= measurementDelay))
    {
      float t;

      if (readAHT20(t))
      {
        temperatureC = t;
        valid = true;
      }
      else
      {
        valid = false;
      }

      measurementPending = false;
    }

    if (!measurementPending && (now - lastRequest >= interval))
    {
      measurementPending = true;
      lastRequest = now;
    }
  }

  float getTemperatureC()
  {
    return temperatureC;
  }

  bool isValid()
  {
    return valid;
  }
}
