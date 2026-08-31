#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>

namespace rotary_encoder
{
  void begin();
  void update();

  int getPosition();
  int getDelta();

  bool isPressed();
  bool wasClicked();

  void reset();
}

#endif
