#include <Arduino.h>
#include "Pins.h"
#include "buzzer.h"

namespace buzzer
{
  static bool active = false;
  static bool beeping = false;
  static bool alarmRunning = false;
  static bool alarmOutput = false;

  static unsigned long beepStart = 0;
  static unsigned long beepDuration = 0;
  static unsigned long lastAlarmToggle = 0;

  static const uint16_t alarmOnTime = 180;
  static const uint16_t alarmOffTime = 180;

  void begin()
  {
    pinMode(Pins::BUZZER, OUTPUT);
    off();
  }

  void update()
  {
    unsigned long now = millis();

    if (alarmRunning)
    {
      uint16_t interval = alarmOutput ? alarmOnTime : alarmOffTime;

      if (now - lastAlarmToggle >= interval)
      {
        lastAlarmToggle = now;
        alarmOutput = !alarmOutput;
        active = alarmOutput;
        Pins::writePin(Pins::BUZZER, alarmOutput);
      }

      return;
    }

    if (beeping && now - beepStart >= beepDuration)
    {
      off();
    }
  }

  void on()
  {
    alarmRunning = false;
    beeping = false;
    alarmOutput = true;
    active = true;
    Pins::writePin(Pins::BUZZER, true);
  }

  void off()
  {
    alarmRunning = false;
    beeping = false;
    alarmOutput = false;
    active = false;
    Pins::writePin(Pins::BUZZER, false);
  }

  void beep(uint16_t durationMs)
  {
    if (alarmRunning)
      return;

    beepDuration = durationMs;
    beepStart = millis();
    beeping = true;
    alarmOutput = true;
    active = true;

    Pins::writePin(Pins::BUZZER, true);
  }

  void startAlarm()
  {
    if (alarmRunning)
      return;

    alarmRunning = true;
    beeping = false;
    alarmOutput = true;
    active = true;
    lastAlarmToggle = millis();

    Pins::writePin(Pins::BUZZER, true);
  }

  void stopAlarm()
  {
    off();
  }

  bool isActive()
  {
    return active;
  }

  bool isAlarmRunning()
  {
    return alarmRunning;
  }
}
