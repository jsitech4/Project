#include <Arduino.h>
#include "battery_level.h"
#include "esp_sleep.h"
#include "Pins.h"

namespace battery_level
{
  static float voltage = 0.0f;
  static uint8_t percentage = 0;
  static int rawValue = 0;

  static unsigned long lastUpdate = 0;
  static unsigned long lastPrint = 0;
  static unsigned long lowBatteryStart = 0;

  static const unsigned long updateInterval = 500;
  static const unsigned long printInterval = 1000;
  static const unsigned long lowBatterySleepDelay = 5000;

  static const float adcRef = 3.3f;
  static const float adcMax = 4095.0f;

  static const float dividerRatio = 1.23f;

  static const float batteryMin = 3.20f;
  static const float batteryMax = 4.0f;
  static const uint8_t lowBatteryPercent = 20;

  static uint8_t voltageToPercent(float v)
  {
    if (v <= batteryMin)
    {
      return 0;
    }

    if (v >= batteryMax)
    {
      return 100;
    }

    return (uint8_t)(((v - batteryMin) / (batteryMax - batteryMin)) * 100.0f);
  }

  static void readBattery()
  {
    uint32_t sum = 0;

    const uint8_t samples = 12;

    for (uint8_t i = 0; i < samples; i++)
    {
      sum += analogRead(Pins::BATTERY_ADC);
      yield();
    }

    rawValue = sum / samples;

    float adcVoltage = (rawValue * adcRef) / adcMax;
    voltage = adcVoltage * dividerRatio;
    percentage = voltageToPercent(voltage);
  }

  static void printBattery()
  {
    // Serial.print("Battery Raw: ");
    // Serial.print(rawValue);
    // Serial.print(" | Voltage: ");
    // Serial.print(voltage, 3);
    // Serial.print(" V | Percentage: ");
    // Serial.print(percentage);
    // Serial.println(" %");
  }

  void begin()
  {
    analogReadResolution(12);
    analogSetPinAttenuation(Pins::BATTERY_ADC, ADC_11db);

    readBattery();

    lastUpdate = millis();
    lastPrint = millis();
    lowBatteryStart = 0;
  }

  void update()
  {
    unsigned long now = millis();

    if (now - lastUpdate >= updateInterval)
    {
      lastUpdate = now;
      readBattery();
    }

    if (now - lastPrint >= printInterval)
    {
      lastPrint = now;
      printBattery();
    }

    if (isLow())
    {
      if (lowBatteryStart == 0)
      {
        lowBatteryStart = now;
      }
    }
    else
    {
      lowBatteryStart = 0;
    }
  }

  float getVoltage()
  {
    return voltage;
  }

  uint8_t getPercentage()
  {
    return percentage;
  }

  int getRaw()
  {
    return rawValue;
  }

  bool isLow()
  {
    return percentage <= lowBatteryPercent;
  }

  bool shouldSleep()
  {
    if (!isLow())
    {
      return false;
    }

    if (lowBatteryStart == 0)
    {
      return false;
    }

    return millis() - lowBatteryStart >= lowBatterySleepDelay;
  }

  void sleepNow()
  {
    // Serial.println("Low battery. Entering deep sleep...");
    // Serial.flush();

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_deep_sleep_start();
  }
}
