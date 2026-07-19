#include <Arduino.h>
#include "led_indicator.h"

namespace led_indicator
{
  static uint8_t ledPin = 2;

  static const uint8_t pwmChannel = 0;
  static const uint16_t pwmFreq = 10000;
  static const uint8_t pwmResolution = 8;

  static int brightness = 0;
  static bool rising = true;
  static bool enabled = true;

  static unsigned long lastUpdate = 0;
  static const unsigned long stepTime = 1;

  static int pulseCount = 0;

  static bool inPause = false;
  static unsigned long pauseStart = 0;
  static const unsigned long pauseDuration = 1000;

  void begin()
  {
    // ledPin = pin;

    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(ledPin, pwmChannel);
    ledcWrite(pwmChannel, 0);
  }

  void update()
  {
    if (!enabled)
    {
      ledcWrite(pwmChannel, 0);
      return;
    }

    unsigned long now = millis();

    if (inPause)
    {
      if (now - pauseStart >= pauseDuration)
      {
        inPause = false;
      }
      else
      {
        return;
      }
    }

    if (now - lastUpdate < stepTime)
    {
      return;
    }

    lastUpdate = now;

    if (rising)
    {
      brightness++;

      if (brightness >= 255)
      {
        brightness = 255;
        rising = false;
      }
    }
    else
    {
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

    ledcWrite(pwmChannel, brightness);
  }

  void setEnabled(bool state)
  {
    enabled = state;

    if (!enabled)
    {
      brightness = 0;
      rising = true;
      pulseCount = 0;
      inPause = false;
      ledcWrite(pwmChannel, 0);
    }
  }

  bool isEnabled()
  {
    return enabled;
  }
}
