#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include "led_indicator.h"
#include "Pins.h"

namespace
{
  Adafruit_NeoPixel *pixel = nullptr;

  constexpr uint16_t BREATHE_PERIOD = 4000;
  constexpr uint8_t MIN_BRIGHTNESS = 0;
  constexpr uint8_t MAX_BRIGHTNESS = 180;

  uint8_t getBreathingLevel()
  {
    float phase = (millis() % BREATHE_PERIOD) / (float)BREATHE_PERIOD;
    float wave = (cosf(phase * 2.0f * PI) + 1.0f) * 0.5f;
    float shaped = wave * wave;
    return MIN_BRIGHTNESS + (uint8_t)((MAX_BRIGHTNESS - MIN_BRIGHTNESS) * shaped);
  }
}

namespace led_indicator
{
  void begin(uint8_t pin)
  {
    if (pixel != nullptr)
    {
      delete pixel;
      pixel = nullptr;
    }

    if (!Pins::valid(pin))
      return;

    pixel = new Adafruit_NeoPixel(1, pin, NEO_GRB + NEO_KHZ800);
    pixel->begin();
    pixel->clear();
    pixel->show();
  }

  void update()
  {
    update("idle");
  }

  void update(const char *command)
  {
    if (pixel == nullptr || command == nullptr)
    {
      return;
    }

    if (strcmp(command, "fault") == 0)
    {
      pixel->setPixelColor(0, pixel->Color(255, 0, 0));
      pixel->show();
      return;
    }

    if (strcmp(command, "active") == 0)
    {
      pixel->setPixelColor(0, pixel->Color(0, 0, 255));
      pixel->show();
      return;
    }

    if (strcmp(command, "off") == 0)
    {
      pixel->clear();
      pixel->show();
      return;
    }

    uint8_t level = getBreathingLevel();
    pixel->setPixelColor(0, pixel->Color(0, level, 0));
    pixel->show();
  }
}
