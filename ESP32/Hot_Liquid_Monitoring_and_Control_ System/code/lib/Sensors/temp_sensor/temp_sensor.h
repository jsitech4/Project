#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <Arduino.h>

namespace temp_sensor
{
  void begin();
  void update();

  float getTemperatureC();
  bool isValid();
}

#endif
