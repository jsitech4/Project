#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <string.h>
#include "led_indicator.h"
#include "Pins.h"

namespace
{
  Adafruit_NeoPixel *pixel = nullptr;

  constexpr uint16_t BREATHE_PERIOD = 4000;
  constexpr uint8_t MIN_BRIGHTNESS = 0;
  constexpr uint8_t MAX_BRIGHTNESS = 180;
  constexpr unsigned long LED_REFRESH_MS = 25;

  uint32_t lastColor = 0xFFFFFFFF;
  char lastCommand[12] = "";
  unsigned long lastRefresh = 0;

  uint8_t getBreathingLevel()
  {
    float phase = (millis() % BREATHE_PERIOD) / (float)BREATHE_PERIOD;
    float wave = (cosf(phase * 2.0f * PI) + 1.0f) * 0.5f;
    float shaped = wave * wave;
    return MIN_BRIGHTNESS + (uint8_t)((MAX_BRIGHTNESS - MIN_BRIGHTNESS) * shaped);
  }

  bool commandChanged(const char *command)
  {
    return strncmp(lastCommand, command, sizeof(lastCommand)) != 0;
  }

  void rememberCommand(const char *command)
  {
    strncpy(lastCommand, command, sizeof(lastCommand) - 1);
    lastCommand[sizeof(lastCommand) - 1] = '\0';
  }

  void pushColor(uint32_t color, const char *command, bool force)
  {
    if (pixel == nullptr || command == nullptr)
      return;

    unsigned long now = millis();

    if (!force && color == lastColor && now - lastRefresh < LED_REFRESH_MS)
      return;

    if (!force && now - lastRefresh < LED_REFRESH_MS)
      return;

    pixel->setPixelColor(0, color);
    pixel->show();

    lastColor = color;
    lastRefresh = now;
    rememberCommand(command);
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

    lastColor = 0xFFFFFFFF;
    lastCommand[0] = '\0';
    lastRefresh = 0;

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
      return;

    bool force = commandChanged(command);

    if (strcmp(command, "fault") == 0)
    {
      pushColor(pixel->Color(255, 0, 0), command, force);
      return;
    }

    if (strcmp(command, "active") == 0)
    {
      pushColor(pixel->Color(0, 0, 255), command, force);
      return;
    }

    if (strcmp(command, "off") == 0)
    {
      pushColor(0, command, force);
      return;
    }

    uint8_t level = getBreathingLevel();
    pushColor(pixel->Color(0, level, 0), "idle", force);
  }
}
