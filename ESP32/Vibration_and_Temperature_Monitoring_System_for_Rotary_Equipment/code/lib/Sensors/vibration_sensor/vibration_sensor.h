#ifndef VIBRATION_SENSOR_H
#define VIBRATION_SENSOR_H

#include <Arduino.h>

namespace vibration_sensor
{
  void begin();
  void update();

  // Sensor 1
  bool isSensor1Ready();

  float getSensor1X();
  float getSensor1Y();
  float getSensor1Z();

  float getSensor1VibrationRMS();

  // Sensor 2
  bool isSensor2Ready();

  float getSensor2X();
  float getSensor2Y();
  float getSensor2Z();

  float getSensor2VibrationRMS();

  // Baselines
  float getSensor1BaseMagnitude();
  float getSensor2BaseMagnitude();

  void recalibrateBaseline();
}

#endif
