#include <Arduino.h>
#include "buzzer.h"
namespace buzzer
{
  static uint8_t buzzerPin = 255;
  static bool outputState = false, alarmMode = false, sequenceActive = false;
  static uint16_t beepOn = 100, beepOff = 100;
  static uint8_t targetCount = 0, doneCount = 0;
  static unsigned long lastChange = 0;
  static void setOutput(bool state)
  {
    outputState = state;
    if (buzzerPin != 255)
      digitalWrite(buzzerPin, state ? HIGH : LOW);
  }
  void begin(uint8_t pin)
  {
    buzzerPin = pin;
    pinMode(buzzerPin, OUTPUT);
    setOutput(false);
  }
  void update()
  {
    unsigned long now = millis();
    if (alarmMode)
    {
      if (now - lastChange >= (outputState ? 200 : 300))
      {
        lastChange = now;
        setOutput(!outputState);
      }
      return;
    }
    if (!sequenceActive)
      return;
    if (outputState && now - lastChange >= beepOn)
    {
      lastChange = now;
      setOutput(false);
      doneCount++;
      if (doneCount >= targetCount)
        sequenceActive = false;
      return;
    }
    if (!outputState && sequenceActive && now - lastChange >= beepOff)
    {
      lastChange = now;
      setOutput(true);
    }
  }
  void beep(uint16_t onTime, uint16_t offTime, uint8_t count)
  {
    beepOn = onTime;
    beepOff = offTime;
    targetCount = count;
    doneCount = 0;
    alarmMode = false;
    sequenceActive = true;
    lastChange = millis();
    setOutput(true);
  }
  void alarm(bool state)
  {
    alarmMode = state;
    sequenceActive = false;
    lastChange = millis();
    if (!state)
      setOutput(false);
  }
  bool isActive() { return alarmMode || sequenceActive || outputState; }
}
