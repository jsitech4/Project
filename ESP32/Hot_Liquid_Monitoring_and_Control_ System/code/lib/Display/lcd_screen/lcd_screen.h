#ifndef LCD_SCREEN_H
#define LCD_SCREEN_H

#include <Arduino.h>

namespace lcd_screen
{
  void begin();
  void update();

  void setScreen(uint8_t index);
  uint8_t getScreen();
}

#endif
