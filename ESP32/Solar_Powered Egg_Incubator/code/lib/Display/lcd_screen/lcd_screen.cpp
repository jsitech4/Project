#include <Arduino.h>
#include <U8g2lib.h>
#include "Pins.h"
#include "lcd_screen.h"
#include "battery_level.h"
#include "solar_level.h"
#include "temp_hum.h"
#include "ultrasonic.h"
#include "rotary_encoder.h"
#include "heater.h"
#include "spinner.h"
#include "humidifier.h"
#include "rtc_clock.h"

namespace lcd_screen
{
  static U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, Pins::LCD_CLK, Pins::LCD_MOSI, Pins::LCD_CS, U8X8_PIN_NONE);

  static const uint32_t HOME_REDRAW_MS = 1000;
  static const uint32_t SCREEN_SLEEP_MS = 30000;

  static bool awake = true;
  static unsigned long lastDraw = 0;
  static unsigned long lastUserActivity = 0;

  static bool processLocked = false;
  static bool tempScreenActive = false;
  static bool screenDirty = true;
  static uint8_t activePriority = PRIORITY_HOME;
  static uint32_t screenStartedAt = 0;
  static uint32_t screenExpiresAt = 0;
  static uint32_t processMinDisplayMs = 0;

  static char activeTitle[24] = "";
  static char activeLines[3][24] = {{0}, {0}, {0}};

  static void copyText(char *dst, size_t dstSize, const char *src)
  {
    if (dstSize == 0)
      return;

    if (src == nullptr)
      src = "";

    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
  }

  static bool timePassed(uint32_t now, uint32_t target)
  {
    return (int32_t)(now - target) >= 0;
  }

  static void drawBattery(uint8_t x, uint8_t y, uint8_t percent)
  {
    u8g2.drawFrame(x, y, 18, 7);
    u8g2.drawBox(x + 18, y + 2, 2, 3);
    uint8_t fill = (percent * 16) / 100;
    u8g2.drawBox(x + 1, y + 1, fill, 5);
  }

  static void drawCenteredText(uint8_t y, const char *text)
  {
    if (text == nullptr || text[0] == '\0')
      return;

    int16_t width = u8g2.getStrWidth(text);
    int16_t x = (128 - width) / 2;
    if (x < 0)
      x = 0;

    u8g2.drawStr((uint8_t)x, y, text);
  }

  static void drawHomeScreen()
  {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    u8g2.drawStr(18, 8, "Egg Incubator");

    drawBattery(106, 2, battery_level::getPercentage());

    u8g2.drawHLine(0, 11, 128);
    u8g2.drawBox(64, 11, 1, 45);

    u8g2.setCursor(0, 20);
    u8g2.print("Temp:");
    u8g2.print(temp_hum::getTemperature(), 1);
    u8g2.print("C");
    u8g2.setCursor(67, 20);
    u8g2.print("Heat:");
    u8g2.print(heater::isOn() ? "ON " : "OFF");

    u8g2.setCursor(0, 31);
    u8g2.print("Hum:");
    u8g2.print(temp_hum::getHumidity(), 0);
    u8g2.print("%");

    u8g2.setCursor(67, 31);
    u8g2.print("Spin:");
    u8g2.print(spinner::isOn() ? "ON " : "OFF");

    u8g2.setCursor(0, 42);
    u8g2.print("Dist:");
    u8g2.print(ultrasonic::getDistanceCm(), 1);
    u8g2.print("cm");

    u8g2.setCursor(67, 42);
    u8g2.print("Humid:");
    u8g2.print(humidifier::isOn() ? "ON" : "OFF");

    u8g2.setCursor(0, 53);
    u8g2.print("Batt:");
    u8g2.print(battery_level::getVoltage(), 2);
    u8g2.print("V");

    u8g2.setCursor(67, 53);
    u8g2.print("Sol:");
    u8g2.print(solar_level::getVoltage(), 1);
    u8g2.print("V");

    u8g2.drawHLine(0, 55, 128);

    u8g2.setCursor(0, 64);
    u8g2.print(rtc_clock::getDate());
    u8g2.setCursor(67, 64);
    u8g2.print(rtc_clock::getTime());
    u8g2.sendBuffer();
  }

  static void drawActiveScreen()
  {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    u8g2.drawFrame(0, 0, 128, 64);
    drawCenteredText(12, activeTitle);
    u8g2.drawHLine(6, 16, 116);

    u8g2.setCursor(6, 29);
    u8g2.print(activeLines[0]);

    u8g2.setCursor(6, 42);
    u8g2.print(activeLines[1]);

    u8g2.setCursor(6, 55);
    u8g2.print(activeLines[2]);

    u8g2.sendBuffer();
  }

  static void setActiveText(const char *title, const char *line1, const char *line2, const char *line3)
  {
    copyText(activeTitle, sizeof(activeTitle), title);
    copyText(activeLines[0], sizeof(activeLines[0]), line1);
    copyText(activeLines[1], sizeof(activeLines[1]), line2);
    copyText(activeLines[2], sizeof(activeLines[2]), line3);
  }

  void begin()
  {
    u8g2.begin();
    u8g2.setContrast(180);
    lastUserActivity = millis();
    lastDraw = 0;
    screenDirty = true;
  }

  void update()
  {
    const uint32_t now = millis();
    const unsigned long encoderActivity = rotary_encoder::lastActivity();

    if (encoderActivity != 0 && encoderActivity != lastUserActivity)
    {
      wake();
      lastUserActivity = encoderActivity;
    }

    if (awake && !isBusy() && now - lastUserActivity >= SCREEN_SLEEP_MS)
    {
      sleep();
    }

    if (!awake)
      return;

    if (tempScreenActive && !processLocked && timePassed(now, screenExpiresAt))
    {
      releaseScreen();
    }

    if (isBusy())
    {
      if (screenDirty)
      {
        drawActiveScreen();
        lastDraw = now;
        screenDirty = false;
      }
      return;
    }

    if (screenDirty || now - lastDraw >= HOME_REDRAW_MS)
    {
      drawHomeScreen();
      lastDraw = now;
      screenDirty = false;
    }
  }

  void setBacklight(bool state)
  {
    if (state)
      wake();
    else
      sleep();
  }

  bool isAwake()
  {
    return awake;
  }

  void wake()
  {
    if (!awake)
    {
      awake = true;
      u8g2.setPowerSave(0);
      digitalWrite(Pins::LCD_PWR, HIGH);
      digitalWrite(Pins::LCD_LED, HIGH);
    }
    lastUserActivity = millis();
    lastDraw = 0;
    screenDirty = true;
  }

  void sleep()
  {
    if (!awake)
      return;

    awake = false;
    digitalWrite(Pins::LCD_PWR, LOW);
    digitalWrite(Pins::LCD_LED, LOW);
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    u8g2.setPowerSave(1);
  }

  bool showMessage(const char *title, const char *line1, const char *line2, const char *line3, uint32_t displayTimeMs, uint8_t priority)
  {
    const uint32_t now = millis();

    if (processLocked && priority <= activePriority)
      return false;

    if (tempScreenActive && priority < activePriority)
      return false;

    if (displayTimeMs == 0)
      displayTimeMs = 1;

    setActiveText(title, line1, line2, line3);
    tempScreenActive = true;
    processLocked = false;
    activePriority = priority;
    screenStartedAt = now;
    screenExpiresAt = now + displayTimeMs;
    processMinDisplayMs = 0;
    screenDirty = true;
    wake();
    return true;
  }

  bool beginProcess(const char *title, const char *line1, const char *line2, const char *line3, uint32_t minDisplayMs, uint8_t priority)
  {
    const uint32_t now = millis();

    if (processLocked && priority <= activePriority)
      return false;

    if (tempScreenActive && priority < activePriority)
      return false;

    setActiveText(title, line1, line2, line3);
    tempScreenActive = true;
    processLocked = true;
    activePriority = priority;
    screenStartedAt = now;
    screenExpiresAt = 0;
    processMinDisplayMs = minDisplayMs;
    screenDirty = true;
    wake();
    return true;
  }

  void endProcess()
  {
    if (!processLocked)
      return;

    const uint32_t now = millis();
    const uint32_t earliestRelease = screenStartedAt + processMinDisplayMs;
    processLocked = false;

    if (processMinDisplayMs > 0 && !timePassed(now, earliestRelease))
    {
      tempScreenActive = true;
      screenExpiresAt = earliestRelease;
    }
    else
    {
      releaseScreen();
    }
  }

  void releaseScreen()
  {
    tempScreenActive = false;
    processLocked = false;
    activePriority = PRIORITY_HOME;
    screenStartedAt = 0;
    screenExpiresAt = 0;
    processMinDisplayMs = 0;
    screenDirty = true;
  }

  bool isBusy()
  {
    return processLocked || tempScreenActive;
  }
}
