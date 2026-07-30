#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H
#include <Arduino.h>
namespace rotary_encoder
{
  void begin(uint8_t pinA, uint8_t pinB, uint8_t pinSW);
  void update();
  int getPosition();
  int getDelta();
  bool isPressed();
  bool wasPressed();
  unsigned long lastActivity();
}
#endif
