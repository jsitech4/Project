#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H
#include <Arduino.h>
namespace ultrasonic_sensor
{
  void begin(uint8_t trigPin, uint8_t echoPin);
  void update();
  float getDistanceCm();
  bool isObjectDetected(float thresholdCm);
}
#endif
