#pragma once

#include <Arduino.h>

namespace config_manager
{
  void begin();
  void update();

  void save();
  void resetDefaults();

  int getRelayPower(uint8_t index);
  void setRelayPower(uint8_t index, int value);

  int getInverterPower();
  void setInverterPower(int value);

  int getSystemPower();
  void setSystemPower(int value);

  int getLoadMarginPercent();
  void setLoadMarginPercent(int value);

  bool isConfigured();
  void setConfigured(bool state);
}
