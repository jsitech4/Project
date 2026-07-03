#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "oled_screen.h"
#include "Pins.h"
#include "battery_level/battery_level.h"
#include "rtc/rtc.h"
#include "wifi_manager/wifi_manager.h"

namespace oled_screen
{
  static U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

  static bool readyScreen = false;
  static bool temporaryScreen = false;
  static bool processBusy = false;
  static bool pendingHomeReturn = false;

  static unsigned long screenStartTime = 0;
  static unsigned long screenDuration = 0;
  static unsigned long lastReadyRefresh = 0;

  static unsigned long readyStartTime = 0;
  static String currentIp = "";
  static bool ready = false;
  static String titleText = "";
  static String lineText1 = "";
  static String lineText2 = "";
  static String lineText3 = "";
  static bool dirty = true;
  static unsigned long lastRender = 0;

  void drawBatteryBar()
  {
    uint8_t percent = battery_level::getPercentage();

    uint8_t x = 116;
    uint8_t y = 1;
    uint8_t w = 10;
    uint8_t h = 5;

    u8g2.drawFrame(x, y, w, h);
    u8g2.drawBox(x + w, y + 1, 1, 3);

    uint8_t fillWidth = (percent * (w - 2)) / 100;
    u8g2.drawBox(x + 1, y + 1, fillWidth, h - 2);
  }

  void drawNetworkIcon()
  {
    uint8_t x = 100;
    uint8_t y = 1;
    bool connected = wifi_manager::isConnected();

    u8g2.drawBox(x + 0, y + 4, 2, 1);
    u8g2.drawBox(x + 3, y + 3, 2, 2);

    if (connected)
    {
      u8g2.drawBox(x + 6, y + 2, 2, 3);
      u8g2.drawBox(x + 9, y + 1, 2, 4);
    }
    else
    {
      u8g2.drawBox(x + 6, y + 2, 2, 3);
      u8g2.drawLine(x - 5, y + 1, x - 2, y + 4);
      u8g2.drawLine(x - 2, y + 1, x - 5, y + 4);
    }
  }

  void drawTopIcons()
  {
    drawNetworkIcon();
    drawBatteryBar();
  }

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
    drawTopIcons();
    u8g2.setFont(u8g2_font_6x12_tf);

    drawCentered(16, titleText, 21);
    u8g2.drawHLine(12, 19, 104);

    drawCentered(32, lineText1, 21);
    drawCentered(46, lineText2, 21);
    drawCentered(60, lineText3, 21);

    u8g2.sendBuffer();

    dirty = false;
    lastRender = millis();
  }

  static void applyReadyScreen()
  {
    if (processBusy || temporaryScreen)
    {
      pendingHomeReturn = true;
      return;
    }

    pendingHomeReturn = false;
    readyScreen = true;
    readyStartTime = millis();
    lastReadyRefresh = 0;

    titleText = "ATTENDANCE SYSTEM";
    lineText1 = "Tap card first";
    lineText2 = rtc::getDate();
    lineText3 = rtc::getTime();

    dirty = true;
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

    if (temporaryScreen && millis() - screenStartTime >= screenDuration)
    {
      temporaryScreen = false;

      if (!processBusy)
        applyReadyScreen();
      else
        pendingHomeReturn = true;
    }

    if (!temporaryScreen && !processBusy && pendingHomeReturn)
      applyReadyScreen();

    if (readyScreen && !temporaryScreen && !processBusy)
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

    render();
  }

  void show(const String &title,
            const String &line1,
            const String &line2,
            const String &line3,
            uint32_t duration)
  {
    titleText = title;
    lineText1 = line1;
    lineText2 = line2;
    lineText3 = line3;

    readyScreen = false;
    pendingHomeReturn = false;
    dirty = true;

    if (duration > 0)
    {
      temporaryScreen = true;
      screenDuration = duration;
      screenStartTime = millis();
    }
    else
    {
      temporaryScreen = false;
      screenDuration = 0;
      screenStartTime = 0;
    }
  }

  void showBoot()
  {
    show("Attendance System", "Fingerprint + RFID", "Starting...", "Please wait", 4000);
  }

  void showReady(const String &ip)
  {
    currentIp = ip;
    applyReadyScreen();
  }

  void showError(const String &message)
  {
    show("System Error", message, "Check hardware", "or storage", 3000);
  }

  void setProcessBusy(bool busy)
  {
    if (processBusy == busy)
      return;

    processBusy = busy;

    if (!processBusy && !temporaryScreen && pendingHomeReturn)
      applyReadyScreen();
  }

  bool isProcessBusy()
  {
    return processBusy;
  }

  bool isTemporaryActive()
  {
    return temporaryScreen;
  }

  bool isReady()
  {
    return ready;
  }
}
