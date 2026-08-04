#include <Arduino.h>
#include "Pins.h"
#include "led_indicator.h"
#include "maintenance_manager/maintenance_manager.h"

namespace led_indicator
{
  static const uint8_t ledPin = Pins::STATUS_LED;

  static const uint8_t pwmChannel = 0;
  static const uint16_t pwmFreq = 10000;
  static const uint8_t pwmResolution = 8;

  static uint8_t brightness = 0;
  static bool rising = true;
  static bool state = false;
  static bool forcedMode = false;

  static unsigned long lastUpdate = 0;
  static unsigned long lastBlink = 0;
  static const uint16_t normalStepTime = 2;
  static const uint16_t warningStepTime = 1;

  static uint8_t pulseCount = 0;
  static bool inPause = false;
  static unsigned long pauseStart = 0;
  static const uint16_t pauseDuration = 1000;

  static void writeBrightness(uint8_t value)
  {
    brightness = value;
    state = brightness > 0;
    ledcWrite(pwmChannel, brightness);
  }

  static void blinkUpdate(uint16_t interval)
  {
    unsigned long now = millis();

    if (now - lastBlink >= interval)
    {
      lastBlink = now;
      writeBrightness(state ? 0 : 255);
    }
  }

  static void breathingUpdate(uint16_t stepTime)
  {
    unsigned long now = millis();

    if (inPause)
    {
      if (now - pauseStart >= pauseDuration)
        inPause = false;
      else
        return;
    }

    if (now - lastUpdate < stepTime)
      return;

    lastUpdate = now;

    if (rising)
    {
      if (brightness < 255)
        brightness++;

      if (brightness >= 255)
      {
        brightness = 255;
        rising = false;
      }
    }
    else
    {
      if (brightness > 0)
        brightness--;

      if (brightness <= 0)
      {
        brightness = 0;
        rising = true;
        pulseCount++;

        if (pulseCount >= 2)
        {
          pulseCount = 0;
          inPause = true;
          pauseStart = now;
        }
      }
    }

    writeBrightness(brightness);
  }

  void begin()
  {
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(ledPin, pwmChannel);
    forcedMode = false;
    writeBrightness(0);
  }

  void update()
  {
    if (forcedMode)
      return;

    if (maintenance_manager::isFault())
    {
      blinkUpdate(120);
      return;
    }

    if (maintenance_manager::isCritical())
    {
      blinkUpdate(300);
      return;
    }

    if (maintenance_manager::isWarning())
    {
      breathingUpdate(warningStepTime);
      return;
    }

    breathingUpdate(normalStepTime);
  }

  void on()
  {
    forcedMode = true;
    writeBrightness(255);
  }

  void off()
  {
    forcedMode = true;
    writeBrightness(0);
  }

  void resume()
  {
    forcedMode = false;
  }

  bool isOn()
  {
    return state;
  }
}
