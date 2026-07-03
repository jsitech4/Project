#pragma once

#include <Arduino.h>

namespace load_relay
{
  void begin();
  void update();

  void setRelay(int relay, bool onInverter);
  void setLoadEnabled(int relay, bool enabled);

  void setAllInverter();
  void setAllPHCN();
  void enableAllLoads();
  void disableAllLoads();

  bool isOnInverter(int relay);
  bool isLoadEnabled(int relay);
  bool isBusy();

  bool getRelay1();
  bool getRelay2();
  bool getRelay3();
  bool getRelay4();
  bool getRelay5();
  bool getRelay6();
}
