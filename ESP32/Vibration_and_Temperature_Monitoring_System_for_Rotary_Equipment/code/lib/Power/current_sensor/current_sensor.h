#ifndef CURRENT_SENSOR_H
#define CURRENT_SENSOR_H

#include <Arduino.h>

namespace current_sensor
{
  void begin();
  void update();

  float getCurrentA();
  float getVoltageRMS();
  int getRawADC();
  bool hasFreshReading();

  void setCalibration(float calibration);
  float getCalibration();
}

#endif
