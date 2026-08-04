#include <Arduino.h>
#include "rotary_encoder.h"
namespace rotary_encoder
{
  static uint8_t encoderA = 255, encoderB = 255, encoderSW = 255;
  static int position = 0, lastPosition = 0;
  static uint8_t lastEncoded = 0;
  static bool buttonState = false, lastButtonReading = false, pressEvent = false;
  static unsigned long lastDebounce = 0, activityTime = 0;
  void begin(uint8_t pinA, uint8_t pinB, uint8_t pinSW)
  {
    encoderA = pinA;
    encoderB = pinB;
    encoderSW = pinSW;
    pinMode(encoderA, INPUT_PULLUP);
    pinMode(encoderB, INPUT_PULLUP);
    pinMode(encoderSW, INPUT_PULLUP);
    uint8_t a = digitalRead(encoderA), b = digitalRead(encoderB);
    lastEncoded = (a << 1) | b;
    activityTime = millis();
  }
  void update()
  {
    uint8_t a = digitalRead(encoderA), b = digitalRead(encoderB);
    uint8_t encoded = (a << 1) | b;
    uint8_t sum = (lastEncoded << 2) | encoded;
    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    {
      position++;
      activityTime = millis();
    }
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    {
      position--;
      activityTime = millis();
    }
    lastEncoded = encoded;
    bool reading = digitalRead(encoderSW) == LOW;
    if (reading != lastButtonReading)
      lastDebounce = millis();
    if (millis() - lastDebounce > 35)
    {
      if (reading != buttonState)
      {
        buttonState = reading;
        activityTime = millis();
        if (buttonState)
          pressEvent = true;
      }
    }
    lastButtonReading = reading;
  }
  int getPosition() { return position / 4; }
  int getDelta()
  {
    int current = getPosition();
    int delta = current - lastPosition;
    lastPosition = current;
    return delta;
  }
  bool isPressed() { return buttonState; }
  bool wasPressed()
  {
    bool e = pressEvent;
    pressEvent = false;
    return e;
  }
  unsigned long lastActivity() { return activityTime; }
}
