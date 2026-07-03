#ifndef OLED_SCREEN_H
#define OLED_SCREEN_H

#include <Arduino.h>

namespace oled_screen
{
  void begin();
  void update();
  void drawBatteryBar();
  void drawNetworkIcon();
  void drawTopIcons();
  void show(const String &title, const String &line1, const String &line2, const String &line3, uint32_t duration = 0);

  void showBoot();
  void showReady(const String &ip);
  void showError(const String &message);

  void setProcessBusy(bool busy);
  bool isProcessBusy();
  bool isTemporaryActive();

  bool isReady();
}

#endif
