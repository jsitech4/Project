#pragma once

namespace temp_sensor
{
  void begin();
  void update();
  float getTemperatureC();
  bool isValid();
}
