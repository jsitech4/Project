#ifndef LCD_SCREEN_H
#define LCD_SCREEN_H
#include <Arduino.h>

namespace lcd_screen {
  enum ScreenPriority : uint8_t {
    PRIORITY_HOME = 0,
    PRIORITY_INFO = 1,
    PRIORITY_PROCESS = 5,
    PRIORITY_ALERT = 10
  };

  void begin();
  void update();

  void setBacklight(bool state);
  bool isAwake();
  void wake();
  void sleep();

  // Timed screen: home screen will not return until displayTimeMs expires.
  bool showMessage(const char *title,
                   const char *line1 = "",
                   const char *line2 = "",
                   const char *line3 = "",
                   uint32_t displayTimeMs = 2500,
                   uint8_t priority = PRIORITY_INFO);

  // Process screen: home screen is locked out until endProcess() is called.
  bool beginProcess(const char *title,
                    const char *line1 = "",
                    const char *line2 = "",
                    const char *line3 = "",
                    uint32_t minDisplayMs = 0,
                    uint8_t priority = PRIORITY_PROCESS);

  void endProcess();
  void releaseScreen();
  bool isBusy();
}
#endif
