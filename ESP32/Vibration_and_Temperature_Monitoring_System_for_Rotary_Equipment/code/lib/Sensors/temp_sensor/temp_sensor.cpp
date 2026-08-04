#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Pins.h"
#include "temp_sensor.h"

namespace temp_sensor
{
  static OneWire oneWire(Pins::DS18B20_DATA);
  static DallasTemperature sensors(&oneWire);

  static float temperatureC = NAN;
  static bool valid = false;

  static bool conversionRequested = false;
  static unsigned long lastRequest = 0;

  static const unsigned long conversionDelay = 750;
  static const unsigned long interval = 1000;

  void begin()
  {
    sensors.begin();
    sensors.setResolution(12);
    sensors.setWaitForConversion(false);

    sensors.requestTemperatures();
    conversionRequested = true;
    lastRequest = millis();
  }

  void update()
  {
    unsigned long now = millis();

    if (conversionRequested && now - lastRequest >= conversionDelay)
    {
      float t = sensors.getTempCByIndex(0);

      valid = t != DEVICE_DISCONNECTED_C && t > -55.0f && t < 125.0f;

      if (valid)
      {
        temperatureC = t;
      }

      conversionRequested = false;
    }

    if (!conversionRequested && now - lastRequest >= interval)
    {
      sensors.requestTemperatures();
      conversionRequested = true;
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
