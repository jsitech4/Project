#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "oled_screen.h"
#include "Pins.h"
#include "rtc/rtc.h"

namespace oled_screen
{
  static U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

  static bool readyScreen = false;
  static unsigned long readyStartTime = 0;
  static String currentIp = "";
  static unsigned long lastReadyRefresh = 0;
  static bool ready = false;
  static String titleText = "";
  static String lineText1 = "";
  static String lineText2 = "";
  static String lineText3 = "";
  static bool dirty = true;
  static unsigned long lastRender = 0;

  static String fitText(const String &text, uint8_t maxChars)
  {
    String value = text;
    value.trim();

    if (value.length() <= maxChars)
      return value;

    if (maxChars <= 2)
      return value.substring(0, maxChars);

    return value.substring(0, maxChars - 2) + "..";
  }

  static void drawCentered(uint8_t y, const String &text, uint8_t maxChars)
  {
    String fitted = fitText(text, maxChars);
    int16_t w = u8g2.getStrWidth(fitted.c_str());
    int16_t x = (128 - w) / 2;

    if (x < 0)
      x = 0;

    u8g2.drawStr(x, y, fitted.c_str());
  }

  static void render()
  {
    if (!ready)
      return;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tf);

    drawCentered(10, titleText, 21);
    u8g2.drawHLine(12, 13, 104);

    drawCentered(29, lineText1, 21);
    drawCentered(43, lineText2, 21);
    drawCentered(57, lineText3, 21);

    u8g2.sendBuffer();

    dirty = false;
    lastRender = millis();
  }

  void begin()
  {
    Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

    ready = u8g2.begin();

    if (!ready)
      return;

    showBoot();
    render();
  }

  void update()
  {
    if (!ready)
      return;

    if (readyScreen)
    {
      if (millis() - lastReadyRefresh >= 1000)
      {
        lastReadyRefresh = millis();

        lineText2 = rtc::getDate();
        lineText3 = rtc::getTime();

        dirty = true;
      }
    }

    if (!dirty)
      return;

    if (millis() - lastRender < 80)
      return;

    lastRender = millis();
    dirty = false;
    render();
  }

  void show(const String &title,
            const String &line1,
            const String &line2,
            const String &line3)
  {
    // Any call to show() other than showReady()
    // automatically exits the Ready screen.

    readyScreen = false;

    titleText = title;
    lineText1 = line1;
    lineText2 = line2;
    lineText3 = line3;

    dirty = true;
  }

  void showBoot()
  {
    show("Attendance System", "Fingerprint + RFID", "Starting...", "Please wait");
  }

  void showReady(const String &ip)
  {
    currentIp = ip;

    readyScreen = true;
    readyStartTime = millis();

    titleText = "ATTENDANCE SYSTEM";
    lineText1 = "Tap card first";
    lineText2 = rtc::getDate();
    lineText3 = rtc::getTime();

    dirty = true;
  }

  void showError(const String &message)
  {
    show("System Error", message, "Check hardware", "or storage");
  }

  bool isReady()
  {
    return ready;
  }
}
