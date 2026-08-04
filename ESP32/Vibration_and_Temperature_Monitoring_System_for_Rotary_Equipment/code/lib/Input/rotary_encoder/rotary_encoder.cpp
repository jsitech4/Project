#include <Arduino.h>
#include "Pins.h"
#include "rotary_encoder.h"

namespace rotary_encoder
{
  static int position = 0;
  static int lastReportedPosition = 0;

  static uint8_t lastState = 0;

  static bool pressed = false;
  static bool clicked = false;

  static bool lastButtonReading = HIGH;
  static bool stableButtonState = HIGH;

  static unsigned long lastButtonChange = 0;
  static const unsigned long debounceMs = 35;

  void begin()
  {
    pinMode(Pins::ENCODER_CLK, INPUT_PULLUP);
    pinMode(Pins::ENCODER_DT, INPUT_PULLUP);
    pinMode(Pins::ENCODER_SW, INPUT_PULLUP);

    lastState = (digitalRead(Pins::ENCODER_CLK) << 1) | digitalRead(Pins::ENCODER_DT);

    position = 0;
    lastReportedPosition = 0;
    pressed = false;
    clicked = false;
    lastButtonReading = digitalRead(Pins::ENCODER_SW);
    stableButtonState = lastButtonReading;
    lastButtonChange = millis();
  }

  void update()
  {
    uint8_t state = (digitalRead(Pins::ENCODER_CLK) << 1) | digitalRead(Pins::ENCODER_DT);

    if (state != lastState)
    {
      uint8_t combined = (lastState << 2) | state;

      if (
          combined == 0b1101 ||
          combined == 0b0100 ||
          combined == 0b0010 ||
          combined == 0b1011)
      {
        position++;
      }
      else if (
          combined == 0b1110 ||
          combined == 0b0111 ||
          combined == 0b0001 ||
          combined == 0b1000)
      {
        position--;
      }

      lastState = state;
    }

    bool reading = digitalRead(Pins::ENCODER_SW);

    if (reading != lastButtonReading)
    {
      lastButtonChange = millis();
      lastButtonReading = reading;
    }

    if ((millis() - lastButtonChange) >= debounceMs)
    {
      if (reading != stableButtonState)
      {
        stableButtonState = reading;
        pressed = stableButtonState == LOW;

        if (pressed)
        {
          clicked = true;
        }
      }
    }
  }

  int getPosition()
  {
    return position / 4;
  }

  int getDelta()
  {
    int current = getPosition();
    int delta = current - lastReportedPosition;

    if (delta != 0)
    {
      lastReportedPosition = current;
    }

    return delta;
  }

  bool isPressed()
  {
    return pressed;
  }

  bool wasClicked()
  {
    bool value = clicked;
    clicked = false;
    return value;
  }

  void reset()
  {
    position = 0;
    lastReportedPosition = 0;
  }
}
