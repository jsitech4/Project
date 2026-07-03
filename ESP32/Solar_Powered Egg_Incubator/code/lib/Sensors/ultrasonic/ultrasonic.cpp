#include <Arduino.h>
#include "ultrasonic.h"

namespace ultrasonic {
static uint8_t trig = 255;
static uint8_t echo = 255;
static float distanceCm = 0;

static const uint32_t READ_INTERVAL_MS = 500;
static const uint32_t TRIGGER_HIGH_US = 10;
static const uint32_t ECHO_TIMEOUT_US = 30000UL;

enum ReadState : uint8_t {
  IDLE,
  TRIGGER_HIGH,
  WAIT_ECHO_RISE,
  WAIT_ECHO_FALL
};

static ReadState state = IDLE;
static uint32_t lastReadMs = 0;
static uint32_t stateStartedUs = 0;
static uint32_t echoStartedUs = 0;

static bool microsPassed(uint32_t now, uint32_t start, uint32_t interval)
{
  return (uint32_t)(now - start) >= interval;
}

void begin(uint8_t trigPin, uint8_t echoPin)
{
  trig = trigPin;
  echo = echoPin;
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  digitalWrite(trig, LOW);
  state = IDLE;
  lastReadMs = 0;
}

void update()
{
  const uint32_t nowMs = millis();
  const uint32_t nowUs = micros();

  switch (state)
  {
  case IDLE:
    if ((uint32_t)(nowMs - lastReadMs) < READ_INTERVAL_MS)
      return;

    lastReadMs = nowMs;
    digitalWrite(trig, HIGH);
    stateStartedUs = nowUs;
    state = TRIGGER_HIGH;
    return;

  case TRIGGER_HIGH:
    if (!microsPassed(nowUs, stateStartedUs, TRIGGER_HIGH_US))
      return;

    digitalWrite(trig, LOW);
    stateStartedUs = micros();
    state = WAIT_ECHO_RISE;
    return;

  case WAIT_ECHO_RISE:
    if (digitalRead(echo) == HIGH)
    {
      echoStartedUs = nowUs;
      state = WAIT_ECHO_FALL;
      return;
    }

    if (microsPassed(nowUs, stateStartedUs, ECHO_TIMEOUT_US))
      state = IDLE;
    return;

  case WAIT_ECHO_FALL:
    if (digitalRead(echo) == LOW)
    {
      const uint32_t duration = nowUs - echoStartedUs;
      distanceCm = duration * 0.0343f / 2.0f;
      state = IDLE;
      return;
    }

    if (microsPassed(nowUs, echoStartedUs, ECHO_TIMEOUT_US))
      state = IDLE;
    return;
  }
}

float getDistanceCm()
{
  return distanceCm;
}

bool isObjectDetected(float thresholdCm)
{
  return distanceCm > 0 && distanceCm <= thresholdCm;
}
}
