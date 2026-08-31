#include "lcd_screen.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <string.h>

#include "Pins.h"
#include "load_relay/load_relay.h"
#include "rotary_encoder/rotary_encoder.h"
#include "shared_var/shared_var.h"
#include "config_manager/config_manager.h"
#include "pzem_sensor/pzem_sensor.h"
#include "load_manager/load_manager.h"
#include "nepa_sense/nepa_sense.h"
#include "inverter_sense/inverter_sense.h"
#include "gpio_expander/gpio_expander.h"
#include "local_server/local_server.h"

U8G2_ST7920_128X64_F_SW_SPI lcd(
    U8G2_R2,
    Pins::LCD_CLK,
    Pins::LCD_MOSI,
    Pins::LCD_CS,
    U8X8_PIN_NONE);

namespace lcd_screen
{
  enum ScreenState
  {
    MENU,
    RELAY_SETUP,
    SYSTEM_SETUP,
    SOURCE_STATUS,
    WIFI_INFO,
    EXIT
  };

  ScreenState currentScreen = MENU;

  unsigned long lastMenuMove = 0;
  unsigned long lastRelayMove = 0;
  unsigned long lastSystemMove = 0;

  const unsigned long menuMoveInterval = 160;
  const unsigned long relayMoveInterval = 110;
  const unsigned long systemMoveInterval = 90;

  int selectedItem = 0;
  int relayIndex = 0;
  int systemIndex = 0;

  bool setupFinished = false;

  const int totalItems = 5;
  const char *menuItems[totalItems] = {
      "Relay Power",
      "System Setup",
      "Source Status",
      "WiFi Info",
      "Exit"};

  const int systemItemCount = 4;
  const char *systemItems[systemItemCount] = {
      "Inverter Limit",
      "NEPA Limit",
      "Load Margin",
      "Save & Exit"};

  void drawCenteredText(int boxX, int boxY, int boxW, int boxH, const char *text)
  {
    int textWidth = lcd.getStrWidth(text);
    int x = boxX + (boxW - textWidth) / 2;
    int y = boxY + (boxH / 2) + 3;

    lcd.setCursor(x, y);
    lcd.print(text);
  }

  void renderScreen(void (*drawFunction)())
  {
    lcd.firstPage();
    do
    {
      drawFunction();
    } while (lcd.nextPage());
  }

  void drawHeader(const char *title)
  {
    lcd.setDrawColor(1);
    lcd.drawFrame(0, 0, 128, 64);

    lcd.setFont(u8g2_font_5x8_tr);
    drawCenteredText(0, 0, 128, 12, "SMART HOME ENERGY");
    drawCenteredText(0, 8, 128, 11, title);

    lcd.drawHLine(0, 18, 128);
  }

  void drawRoundedFrame(int x, int y, int w, int h, int r)
  {
    lcd.drawHLine(x + r, y, w - 2 * r);
    lcd.drawHLine(x + r, y + h - 1, w - 2 * r);
    lcd.drawVLine(x, y + r, h - 2 * r);
    lcd.drawVLine(x + w - 1, y + r, h - 2 * r);

    lcd.drawCircle(x + r, y + r, r, U8G2_DRAW_UPPER_LEFT);
    lcd.drawCircle(x + w - r - 1, y + r, r, U8G2_DRAW_UPPER_RIGHT);
    lcd.drawCircle(x + r, y + h - r - 1, r, U8G2_DRAW_LOWER_LEFT);
    lcd.drawCircle(x + w - r - 1, y + h - r - 1, r, U8G2_DRAW_LOWER_RIGHT);
  }

  void drawSwitchStatus(int x, int y, int relayNum, bool onInverter, bool enabled)
  {
    drawRoundedFrame(x, y - 6, 33, 13, 3);

    if (!enabled)
    {
      lcd.setFont(u8g2_font_5x8_tr);
      lcd.setCursor(x + 9, y + 4);
      lcd.print("OFF");
      return;
    }

    char buf[4];
    itoa(relayNum, buf, 10);

    if (onInverter)
    {
      lcd.drawDisc(x + 25, y, 4);

      lcd.setFont(u8g2_font_4x6_tr);
      uint8_t w = lcd.getStrWidth(buf);

      lcd.setCursor(x + 25 - (w / 2), y + 3);
      lcd.setDrawColor(0);
      lcd.print(buf);

      lcd.setDrawColor(1);
      lcd.setFont(u8g2_font_5x8_tr);
      lcd.setCursor(x + 2, y + 4);
      lcd.print("INV");
    }
    else
    {
      lcd.drawDisc(x + 7, y, 4);

      lcd.setFont(u8g2_font_4x6_tr);
      uint8_t w = lcd.getStrWidth(buf);

      lcd.setCursor(x + 7 - (w / 2), y + 3);
      lcd.setDrawColor(0);
      lcd.print(buf);

      lcd.setDrawColor(1);
      lcd.setFont(u8g2_font_5x8_tr);
      lcd.setCursor(x + 13, y + 4);
      lcd.print("GRD");
    }
  }

  void drawValueBox(int x, int y, const char *title, const char *value)
  {
    lcd.setDrawColor(1);
    lcd.drawBox(x, y, 50, 9);
    lcd.drawFrame(x, y, 50, 20);

    lcd.setDrawColor(0);
    lcd.setFont(u8g2_font_5x8_tr);
    drawCenteredText(x, y, 50, 9, title);

    lcd.setDrawColor(1);
    drawCenteredText(x, y + 10, 50, 10, value);
  }

  void drawLoadStatusScreen()
  {
    drawHeader(load_manager::getDecisionText());

    drawSwitchStatus(6, 30, 1, load_relay::getRelay1(), load_relay::isLoadEnabled(0));
    drawSwitchStatus(47, 30, 2, load_relay::getRelay2(), load_relay::isLoadEnabled(1));
    drawSwitchStatus(88, 30, 3, load_relay::getRelay3(), load_relay::isLoadEnabled(2));
    drawSwitchStatus(6, 51, 4, load_relay::getRelay4(), load_relay::isLoadEnabled(3));
    drawSwitchStatus(47, 51, 5, load_relay::getRelay5(), load_relay::isLoadEnabled(4));
    drawSwitchStatus(88, 51, 6, load_relay::getRelay6(), load_relay::isLoadEnabled(5));
  }

  void drawConsumptionScreen()
  {
    char voltageText[16];
    char currentText[16];
    char powerText[16];
    char energyText[16];

    snprintf(voltageText, sizeof(voltageText), "%.1fV", pzem_sensor::getVoltage());
    snprintf(currentText, sizeof(currentText), "%.2fA", pzem_sensor::getCurrent());
    snprintf(powerText, sizeof(powerText), "%.0fW", pzem_sensor::getPower());
    snprintf(energyText, sizeof(energyText), "%.2fkWh", pzem_sensor::getEnergy());

    drawHeader(pzem_sensor::hasValidData() ? "CONSUMPTION" : "PZEM ERROR");

    drawValueBox(10, 20, "VOLT", voltageText);
    drawValueBox(69, 20, "CURR", currentText);
    drawValueBox(10, 42, "POWER", powerText);
    drawValueBox(69, 42, "ENERGY", energyText);
  }

  void drawSettingsScreen()
  {
    lcd.setDrawColor(1);
    lcd.setFont(u8g2_font_ncenB08_tr);

    drawCenteredText(0, 0, 128, 11, "SETTINGS");
    lcd.drawHLine(33, 12, 63);

    lcd.setFont(u8g2_font_5x8_tr);

    for (int i = 0; i < totalItems; i++)
    {
      int y = 22 + (i * 9);

      if (i == selectedItem)
      {
        lcd.drawBox(5, y - 8, 118, 10);
        lcd.setDrawColor(0);
      }

      lcd.setCursor(10, y);
      lcd.print(menuItems[i]);

      lcd.setDrawColor(1);
    }
  }

  void drawRelaySetupScreen()
  {
    lcd.setDrawColor(1);
    lcd.setFont(u8g2_font_6x10_tr);

    drawCenteredText(0, 0, 128, 11, "RELAY POWER SETUP");
    lcd.drawHLine(13, 12, 100);

    lcd.setFont(u8g2_font_5x8_tr);

    for (int i = 0; i < 6; i++)
    {
      int y = 21 + (i * 7);

      if (i == relayIndex)
      {
        lcd.drawBox(0, y - 7, 128, 8);
        lcd.setDrawColor(0);
      }

      lcd.setCursor(5, y);
      lcd.print("Load ");
      lcd.print(i + 1);

      lcd.setCursor(72, y);
      lcd.print(config_manager::getRelayPower(i));
      lcd.print("W");

      lcd.setDrawColor(1);
    }
  }

  void drawSystemSettingsScreen()
  {
    lcd.setDrawColor(1);
    lcd.setFont(u8g2_font_6x10_tr);

    drawCenteredText(0, 0, 128, 11, "SYSTEM SETUP");
    lcd.drawHLine(20, 12, 90);

    lcd.setFont(u8g2_font_5x8_tr);

    for (int i = 0; i < systemItemCount; i++)
    {
      int y = 22 + (i * 10);

      if (i == systemIndex)
      {
        lcd.drawBox(0, y - 8, 128, 11);
        lcd.setDrawColor(0);
      }

      lcd.setCursor(5, y);
      lcd.print(systemItems[i]);

      lcd.setCursor(82, y);

      if (i == 0)
      {
        lcd.print(config_manager::getInverterPower());
        lcd.print("W");
      }
      else if (i == 1)
      {
        lcd.print(config_manager::getSystemPower());
        lcd.print("W");
      }
      else if (i == 2)
      {
        lcd.print(config_manager::getLoadMarginPercent());
        lcd.print("%");
      }
      else
      {
        lcd.print("OK");
      }

      lcd.setDrawColor(1);
    }
  }

  void drawSourceStatusScreen()
  {
    drawHeader("SOURCE STATUS");

    lcd.setFont(u8g2_font_6x10_tr);
    lcd.setCursor(8, 30);
    lcd.print("GRID: ");
    lcd.print(nepa_sense::isAvailable() ? "AVAILABLE" : "OFF");

    lcd.setCursor(8, 43);
    lcd.print("INV : ");
    lcd.print(inverter_sense::isAvailable() ? "AVAILABLE" : "OFF");

    lcd.setCursor(8, 56);
    lcd.print("PCA : ");
    lcd.print(gpio_expander::isReady() ? "READY" : "ERROR");
  }

  void drawWifiInfoScreen()
  {
    drawHeader("WIFI SERVER");

    lcd.setFont(u8g2_font_5x8_tr);
    drawCenteredText(0, 24, 128, 10, "SSID: SHEMS-Controller");
    drawCenteredText(0, 36, 128, 10, "PASS: 12345678");
    drawCenteredText(0, 48, 128, 10, local_server::getIpAddress());
  }

  void updateMenu()
  {
    int dir = rotary_encoder::getDirection();
    unsigned long now = millis();

    if (dir != 0 && now - lastMenuMove >= menuMoveInterval)
    {
      selectedItem += dir > 0 ? 1 : -1;
      selectedItem = (selectedItem + totalItems) % totalItems;
      lastMenuMove = now;
    }
  }

  void relayPowerSetup()
  {
    unsigned long now = millis();
    int dir = rotary_encoder::getDirection();

    if (dir != 0 && now - lastRelayMove >= relayMoveInterval)
    {
      int value = config_manager::getRelayPower(relayIndex);
      value += dir > 0 ? 50 : -50;
      config_manager::setRelayPower(relayIndex, value);
      lastRelayMove = now;
    }

    if (rotary_encoder::wasPressed())
    {
      relayIndex++;

      if (relayIndex >= 6)
      {
        relayIndex = 0;
        config_manager::save();
        setupFinished = true;
        currentScreen = MENU;
        rotary_encoder::lockUntilRelease();
        return;
      }

      rotary_encoder::lockUntilRelease();
    }

    renderScreen(drawRelaySetupScreen);
  }

  void systemSettings()
  {
    unsigned long now = millis();
    int dir = rotary_encoder::getDirection();

    if (dir != 0 && now - lastSystemMove >= systemMoveInterval)
    {
      if (systemIndex == 0)
      {
        int value = config_manager::getInverterPower();
        value += dir > 0 ? 100 : -100;
        config_manager::setInverterPower(value);
      }
      else if (systemIndex == 1)
      {
        int value = config_manager::getSystemPower();
        value += dir > 0 ? 100 : -100;
        config_manager::setSystemPower(value);
      }
      else if (systemIndex == 2)
      {
        int value = config_manager::getLoadMarginPercent();
        value += dir > 0 ? 1 : -1;
        config_manager::setLoadMarginPercent(value);
      }

      lastSystemMove = now;
    }

    if (rotary_encoder::wasPressed())
    {
      systemIndex++;

      if (systemIndex >= systemItemCount)
      {
        systemIndex = 0;
        config_manager::save();
        setupFinished = true;
        currentScreen = MENU;
        rotary_encoder::lockUntilRelease();
        return;
      }

      rotary_encoder::lockUntilRelease();
    }

    renderScreen(drawSystemSettingsScreen);
  }

  void begin()
  {
    lcd.begin();
  }

  void update(const char *command)
  {
    if (currentScreen == RELAY_SETUP)
    {
      relayPowerSetup();
      return;
    }

    if (currentScreen == SYSTEM_SETUP)
    {
      systemSettings();
      return;
    }

    if (currentScreen == SOURCE_STATUS)
    {
      renderScreen(drawSourceStatusScreen);

      if (rotary_encoder::wasPressed())
      {
        currentScreen = MENU;
        rotary_encoder::lockUntilRelease();
      }

      return;
    }

    if (currentScreen == WIFI_INFO)
    {
      renderScreen(drawWifiInfoScreen);

      if (rotary_encoder::wasPressed())
      {
        currentScreen = MENU;
        rotary_encoder::lockUntilRelease();
      }

      return;
    }

    if (currentScreen == EXIT)
    {
      shared_var::settingsMode = false;
      currentScreen = MENU;
      selectedItem = 0;
      rotary_encoder::lockUntilRelease();
      return;
    }

    if (strcmp(command, "loadStatus") == 0)
    {
      renderScreen(drawLoadStatusScreen);
      return;
    }

    if (strcmp(command, "loadConsumption") == 0)
    {
      renderScreen(drawConsumptionScreen);
      return;
    }

    if (strcmp(command, "settings") == 0)
    {
      updateMenu();
      renderScreen(drawSettingsScreen);

      if (rotary_encoder::wasPressed() && shared_var::settingsMode)
      {
        if (selectedItem == 0)
          currentScreen = RELAY_SETUP;
        else if (selectedItem == 1)
          currentScreen = SYSTEM_SETUP;
        else if (selectedItem == 2)
          currentScreen = SOURCE_STATUS;
        else if (selectedItem == 3)
          currentScreen = WIFI_INFO;
        else if (selectedItem == 4)
          currentScreen = EXIT;

        rotary_encoder::lockUntilRelease();
      }
    }
  }

  int getSelectedItem()
  {
    return selectedItem;
  }

  bool isRelaySetupFinished()
  {
    return setupFinished;
  }

  void resetRelaySetup()
  {
    setupFinished = false;
  }

  int getRelayPower(int relay)
  {
    if (relay < 0 || relay > 5)
      return 0;

    return config_manager::getRelayPower(relay);
  }

  int getInverterPower()
  {
    return config_manager::getInverterPower();
  }
}
