#include <Arduino.h>
#include "Pins.h"
#include "rotary_encoder.h"

namespace rotary_encoder
{
  const int encoderA = Pins::ENCODER_A;
  const int encoderB = Pins::ENCODER_B;
  const int encoderSw = Pins::ENCODER_SW;

  int lastEncoded = 0;
  int rawPosition = 0;
  int stepPosition = 0;
  int direction = 0;

  bool lastReading = HIGH;
  bool stableButtonState = HIGH;
  bool lastStableButtonState = HIGH;
  bool pressEvent = false;
  bool inputLocked = false;

  unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 40;

  unsigned long holdStart = 0;
  bool holdTriggered = false;

  void begin()
  {
    pinMode(encoderA, INPUT_PULLUP);
    pinMode(encoderB, INPUT_PULLUP);
    pinMode(encoderSw, INPUT_PULLUP);

    lastEncoded = (digitalRead(encoderA) << 1) | digitalRead(encoderB);
  }

  void update()
  {
    int encoded = (digitalRead(encoderA) << 1) | digitalRead(encoderB);
    int sum = (lastEncoded << 2) | encoded;

    if (sum == 0b0001 || sum == 0b0111 || sum == 0b1110 || sum == 0b1000)
      rawPosition++;
    else if (sum == 0b0010 || sum == 0b0100 || sum == 0b1101 || sum == 0b1011)
      rawPosition--;

    lastEncoded = encoded;

    int newStep = rawPosition / 4;

    if (newStep != stepPosition)
    {
      direction = newStep > stepPosition ? 1 : -1;
      stepPosition = newStep;
    }

    bool reading = digitalRead(encoderSw);
    unsigned long now = millis();

    if (reading != lastReading)
      lastDebounceTime = now;

    if (now - lastDebounceTime > debounceDelay)
    {
      if (reading != stableButtonState)
      {
        stableButtonState = reading;

        if (stableButtonState == LOW && lastStableButtonState == HIGH && !inputLocked)
          pressEvent = true;

        lastStableButtonState = stableButtonState;
      }
    }

    lastReading = reading;

    if (stableButtonState == HIGH)
    {
      inputLocked = false;
      holdStart = 0;
      holdTriggered = false;
    }
  }

  int getDirection()
  {
    int temp = direction;
    direction = 0;
    return temp;
  }

  bool wasPressed()
  {
    if (inputLocked)
      return false;

    bool temp = pressEvent;
    pressEvent = false;

    return temp;
  }

  bool isPressed()
  {
    return stableButtonState == LOW;
  }

  bool isHeld(unsigned long holdTime)
  {
    if (!isPressed())
      return false;

    if (holdStart == 0)
      holdStart = millis();

    if (!holdTriggered && millis() - holdStart >= holdTime)
    {
      holdTriggered = true;
      inputLocked = true;
      pressEvent = false;
      return true;
    }

    return false;
  }

  void lockUntilRelease()
  {
    inputLocked = true;
    pressEvent = false;
  }
}
