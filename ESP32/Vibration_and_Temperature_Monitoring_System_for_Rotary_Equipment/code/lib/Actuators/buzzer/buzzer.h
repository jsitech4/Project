#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

namespace buzzer
{
  void begin();
  void update();

  void on();
  void off();

  void beep(uint16_t durationMs);

  void startAlarm();
  void stopAlarm();

  bool isActive();
  bool isAlarmRunning();
}

#endif
