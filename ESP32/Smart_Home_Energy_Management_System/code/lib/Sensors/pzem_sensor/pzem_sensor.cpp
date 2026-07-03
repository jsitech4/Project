#include <Arduino.h>
#include <PZEM004Tv30.h>
#include "Pins.h"
#include "pzem_sensor.h"

namespace pzem_sensor
{
  PZEM004Tv30 pzem(Serial2, Pins::PZEM_RX, Pins::PZEM_TX);

  float voltage = 0.0f;
  float current = 0.0f;
  float power = 0.0f;
  float energy = 0.0f;
  float frequency = 0.0f;
  float powerFactor = 0.0f;

  bool ready = false;
  bool validData = false;

  unsigned long lastUpdate = 0;
  const unsigned long normalUpdateInterval = 1000;
  const unsigned long retryUpdateInterval = 3000;
  uint8_t consecutiveFailures = 0;

  bool validNumber(float value)
  {
    return !isnan(value) && isfinite(value) && value >= 0.0f;
  }

  void clearLiveValues()
  {
    voltage = 0.0f;
    current = 0.0f;
    power = 0.0f;
  }

  void begin()
  {
    Serial2.begin(9600, SERIAL_8N1, Pins::PZEM_RX, Pins::PZEM_TX);
    ready = true;
    validData = false;
    lastUpdate = 0;
    consecutiveFailures = 0;
  }

  void update()
  {
    unsigned long now = millis();
    unsigned long interval = validData ? normalUpdateInterval : retryUpdateInterval;

    if (lastUpdate != 0 && now - lastUpdate < interval)
      return;

    lastUpdate = now;

    // Important: pzem.voltage() performs one full register read inside the library.
    // If the PZEM is missing, every getter can wait for the serial timeout. So try
    // voltage first and do not call the other getters after a failed read.
    float v = pzem.voltage();

    if (!validNumber(v))
    {
      validData = false;
      clearLiveValues();

      if (consecutiveFailures < 255)
        consecutiveFailures++;

      return;
    }

    float c = pzem.current();
    float p = pzem.power();
    float e = pzem.energy();
    float f = pzem.frequency();
    float pf = pzem.pf();

    bool ok = validNumber(c) && validNumber(p);

    if (!ok)
    {
      validData = false;
      clearLiveValues();

      if (consecutiveFailures < 255)
        consecutiveFailures++;

      return;
    }

    voltage = v;
    current = c;
    power = p;

    if (validNumber(e))
      energy = e;

    if (validNumber(f))
      frequency = f;

    if (!isnan(pf) && isfinite(pf))
      powerFactor = pf;

    validData = true;
    consecutiveFailures = 0;
  }

  float getVoltage()
  {
    return voltage;
  }

  float getCurrent()
  {
    return current;
  }

  float getPower()
  {
    return power;
  }

  float getEnergy()
  {
    return energy;
  }

  float getFrequency()
  {
    return frequency;
  }

  float getPowerFactor()
  {
    return powerFactor;
  }

  int getTotalPower()
  {
    return (int)(power + 0.5f);
  }

  bool isReady()
  {
    return ready;
  }

  bool hasValidData()
  {
    return validData;
  }

  void resetEnergy()
  {
    pzem.resetEnergy();
    energy = 0.0f;
  }
}
