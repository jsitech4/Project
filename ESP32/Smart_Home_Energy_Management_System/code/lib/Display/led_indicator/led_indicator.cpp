#include <Arduino.h>
#include "led_indicator.h"

#if defined(ESP32)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

namespace led_indicator
{
  int ledPin = 2;

  const int pwmChannel = 7;
  const int pwmFreq = 5000;
  const int pwmResolution = 8;

  volatile int brightness = 0;
  volatile bool rising = true;

  unsigned long lastUpdate = 0;
  const unsigned long stepTime = 4;

  volatile int pulseCount = 0;

  volatile bool inPause = false;
  unsigned long pauseStart = 0;
  const unsigned long pauseDuration = 700;

  volatile bool enabled = true;

#if defined(ESP32)
  TaskHandle_t ledTaskHandle = nullptr;
  bool taskRunning = false;
#endif

  void writeBrightness(int value)
  {
    static int lastWritten = -1;

    if (value == lastWritten)
      return;

    ledcWrite(pwmChannel, value);
    lastWritten = value;
  }

  void resetBreathState()
  {
    brightness = 0;
    rising = true;
    pulseCount = 0;
    inPause = false;
    lastUpdate = millis();
    pauseStart = 0;
    writeBrightness(0);
  }

  void serviceBreath()
  {
    if (!enabled)
    {
      resetBreathState();
      return;
    }

    unsigned long now = millis();

    if (inPause)
    {
      if (now - pauseStart < pauseDuration)
        return;

      inPause = false;
      lastUpdate = now;
    }

    if (now - lastUpdate < stepTime)
      return;

    lastUpdate = now;

    int nextBrightness = brightness;

    if (rising)
    {
      nextBrightness += 8;

      if (nextBrightness >= 255)
      {
        nextBrightness = 255;
        rising = false;
      }
    }
    else
    {
      nextBrightness -= 10;

      if (nextBrightness <= 0)
      {
        nextBrightness = 0;
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

    brightness = nextBrightness;
    writeBrightness(nextBrightness);
  }

#if defined(ESP32)
  void ledTask(void *parameter)
  {
    (void)parameter;

    for (;;)
    {
      serviceBreath();
      vTaskDelay(pdMS_TO_TICKS(2));
    }
  }
#endif

  void begin(int pin)
  {
    ledPin = pin;

    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(ledPin, pwmChannel);
    ledcWrite(pwmChannel, 0);
    resetBreathState();

#if defined(ESP32)
    if (ledTaskHandle == nullptr)
    {
      BaseType_t ok = xTaskCreatePinnedToCore(
          ledTask,
          "HeartbeatLED",
          2048,
          nullptr,
          2,
          &ledTaskHandle,
          1);

      taskRunning = (ok == pdPASS);
    }
#endif
  }

  void update()
  {
#if defined(ESP32)
    if (taskRunning)
      return;
#endif

    serviceBreath();
  }

  void setEnabled(bool state)
  {
    enabled = state;

    if (!enabled)
      resetBreathState();
  }

  bool isEnabled()
  {
    return enabled;
  }
}
