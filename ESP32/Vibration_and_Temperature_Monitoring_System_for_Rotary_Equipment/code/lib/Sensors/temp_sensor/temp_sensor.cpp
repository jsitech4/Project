#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MAX31865.h>

#include "Pins.h"
#include "temp_sensor.h"

namespace temp_sensor
{
  static Adafruit_MAX31865 max31865(
      Pins::SPI_CS,
      Pins::SPI_MOSI,
      Pins::SPI_MISO,
      Pins::SPI_SCK);

  static float temperatureC = NAN;
  static bool valid = false;

  static unsigned long lastRead = 0;

  static const unsigned long interval = 1000;

  void begin()
  {
    // Start SPI using your project's SPI pins
    SPI.begin(
        Pins::SPI_SCK,
        Pins::SPI_MISO,
        Pins::SPI_MOSI,
        Pins::SPI_CS);

    pinMode(Pins::SPI_CS, OUTPUT);
    digitalWrite(Pins::SPI_CS, HIGH);

    // MAX31865:
    // 3-wire PT100
    if (!max31865.begin(MAX31865_3WIRE))
    {
      valid = false;
      return;
    }

    // Clear any previous fault
    max31865.clearFault();

    // First reading
    float t = max31865.temperature(
        100.0f, // PT100 nominal resistance
        430.0f  // MAX31865 reference resistor
    );

    uint8_t fault = max31865.readFault();

    if (fault == 0 &&
        !isnan(t) &&
        t > -200.0f &&
        t < 850.0f)
    {
      temperatureC = t;
      valid = true;
    }
    else
    {
      valid = false;
      max31865.clearFault();
    }

    lastRead = millis();
  }

  void update()
  {
    unsigned long now = millis();

    if (now - lastRead < interval)
      return;

    lastRead = now;

    max31865.clearFault();

    float t = max31865.temperature(
        100.0f, // PT100
        430.0f  // RREF
    );

    uint8_t fault = max31865.readFault();

    bool readingValid =
        fault == 0 &&
        !isnan(t) &&
        t > -200.0f &&
        t < 850.0f;

    if (readingValid)
    {
      temperatureC = t;
      valid = true;
    }
    else
    {
      valid = false;
      max31865.clearFault();
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
