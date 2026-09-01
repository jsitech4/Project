#ifndef VIBRATION_SENSOR_H
#define VIBRATION_SENSOR_H

#include <Arduino.h>

namespace vibration_sensor
{
  void begin();
  void update();

  // Sensor 1
  float getSensor1X();
  float getSensor1Y();
  float getSensor1Z();
  float getSensor1VibrationRMS();
  float getSensor1BaseMagnitude();
  bool isSensor1Ready();
  void recalibrateSensor1();

  // Sensor 2
  float getSensor2X();
  float getSensor2Y();
  float getSensor2Z();
  float getSensor2VibrationRMS();
  float getSensor2BaseMagnitude();
  bool isSensor2Ready();
  void recalibrateSensor2();

  // Overall status
  bool isReady();
}

#endif
