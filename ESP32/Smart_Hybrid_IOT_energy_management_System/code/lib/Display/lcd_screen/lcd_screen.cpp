#include "lcd_screen.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
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

U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R2, Pins::LCD_CS, U8X8_PIN_NONE);

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

  const unsigned long menuMoveInterval = 150;
  const unsigned long relayMoveInterval = 100;
  const unsigned long systemMoveInterval = 120;

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

  const int systemItemCount = 3;
  const char *systemItems[systemItemCount] = {
      "Inverter Power",
      "Load Margin",
      "Save & Exit"};

  void drawCenteredText(int boxX, int boxY, int boxW, int boxH, const char *text)
  {
    int textWidth = u8g2.getStrWidth(text);
    int x = boxX + (boxW - textWidth) / 2;
    int y = boxY + (boxH / 2) + 3;

    u8g2.setCursor(x, y);
    u8g2.print(text);
  }

  void renderScreen(void (*drawFunction)())
  {
    u8g2.firstPage();
    do
    {
      drawFunction();
    } while (u8g2.nextPage());
  }

  void drawHeader(const char *title)
  {
    u8g2.setDrawColor(1);
    u8g2.drawFrame(0, 0, 128, 64);

    u8g2.setFont(u8g2_font_5x8_tr);
    drawCenteredText(0, 0, 128, 9, "SHIOTEMS");
    drawCenteredText(0, 10, 128, 9, title);

    u8g2.drawHLine(0, 18, 128);
  }

  void drawRoundedFrame(int x, int y, int w, int h, int r)
  {
    u8g2.drawHLine(x + r, y, w - 2 * r);
    u8g2.drawHLine(x + r, y + h - 1, w - 2 * r);
    u8g2.drawVLine(x, y + r, h - 2 * r);
    u8g2.drawVLine(x + w - 1, y + r, h - 2 * r);

    u8g2.drawCircle(x + r, y + r, r, U8G2_DRAW_UPPER_LEFT);
    u8g2.drawCircle(x + w - r - 1, y + r, r, U8G2_DRAW_UPPER_RIGHT);
    u8g2.drawCircle(x + r, y + h - r - 1, r, U8G2_DRAW_LOWER_LEFT);
    u8g2.drawCircle(x + w - r - 1, y + h - r - 1, r, U8G2_DRAW_LOWER_RIGHT);
  }

  void drawSwitchStatus(int x, int y, int relayNum, bool onInverter, bool enabled)
  {
    drawRoundedFrame(x, y - 6, 33, 13, 3);

    if (!enabled)
    {
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.setCursor(x + 9, y + 4);
      u8g2.print("OFF");
      return;
    }

    char buf[4];
    itoa(relayNum, buf, 10);

    if (onInverter)
    {
      u8g2.drawDisc(x + 25, y, 4);

      u8g2.setFont(u8g2_font_4x6_tr);
      uint8_t w = u8g2.getStrWidth(buf);

      u8g2.setCursor(x + 25 - (w / 2), y + 3);
      u8g2.setDrawColor(0);
      u8g2.print(buf);

      u8g2.setDrawColor(1);
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.setCursor(x + 2, y + 4);
      u8g2.print("INV");
    }
    else
    {
      u8g2.drawDisc(x + 7, y, 4);

      u8g2.setFont(u8g2_font_4x6_tr);
      uint8_t w = u8g2.getStrWidth(buf);

      u8g2.setCursor(x + 7 - (w / 2), y + 3);
      u8g2.setDrawColor(0);
      u8g2.print(buf);

      u8g2.setDrawColor(1);
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.setCursor(x + 13, y + 4);
      u8g2.print("NEP");
    }
  }

  void drawValueBox(int x, int y, const char *title, const char *value)
  {
    u8g2.setDrawColor(1);
    u8g2.drawBox(x, y, 50, 9);
    u8g2.drawFrame(x, y, 50, 20);

    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_5x8_tr);
    drawCenteredText(x, y, 50, 9, title);

    u8g2.setDrawColor(1);
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
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_ncenB08_tr);

    drawCenteredText(0, 0, 128, 11, "SETTINGS");
    u8g2.drawHLine(33, 9, 63);

    u8g2.setFont(u8g2_font_5x8_tr);

    for (int i = 0; i < totalItems; i++)
    {
      int y = 22 + (i * 9);

      if (i == selectedItem)
      {
        u8g2.drawBox(5, y - 8, 118, 10);
        u8g2.setDrawColor(0);
      }

      u8g2.setCursor(10, y);
      u8g2.print(menuItems[i]);

      u8g2.setDrawColor(1);
    }
  }

  void drawRelaySetupScreen()
  {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_6x10_tr);

    drawCenteredText(0, 0, 128, 11, "RELAY POWER SETUP");
    u8g2.drawHLine(13, 12, 100);

    u8g2.setFont(u8g2_font_5x8_tr);

    for (int i = 0; i < 6; i++)
    {
      int y = 21 + (i * 7);

      if (i == relayIndex)
      {
        u8g2.drawBox(0, y - 7, 128, 8);
        u8g2.setDrawColor(0);
      }

      u8g2.setCursor(5, y);
      u8g2.print("Load ");
      u8g2.print(i + 1);

      u8g2.setCursor(72, y);
      u8g2.print(config_manager::getRelayPower(i));
      u8g2.print("W");

      u8g2.setDrawColor(1);
    }
  }

  void drawSystemSettingsScreen()
  {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_6x10_tr);

    drawCenteredText(0, 0, 128, 11, "SYSTEM SETUP");
    u8g2.drawHLine(20, 12, 90);

    u8g2.setFont(u8g2_font_5x8_tr);

    for (int i = 0; i < systemItemCount; i++)
    {
      int y = 25 + (i * 13);

      if (i == systemIndex)
      {
        u8g2.drawBox(0, y - 8, 128, 11);
        u8g2.setDrawColor(0);
      }

      u8g2.setCursor(5, y);
      u8g2.print(systemItems[i]);

      u8g2.setCursor(90, y);

      if (i == 0)
      {
        u8g2.print(config_manager::getInverterPower());
        u8g2.print("W");
      }
      else if (i == 1)
      {
        u8g2.print(config_manager::getLoadMarginPercent());
        u8g2.print("%");
      }
      else
      {
        u8g2.print("OK");
      }

      u8g2.setDrawColor(1);
    }
  }

  void drawSourceStatusScreen()
  {
    drawHeader("SOURCE STATUS");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setCursor(8, 30);
    u8g2.print("GRID: ");
    u8g2.print(nepa_sense::isAvailable() ? "AVAILABLE" : "OFF");

    u8g2.setCursor(8, 43);
    u8g2.print("INV : ");
    u8g2.print(inverter_sense::isAvailable() ? "AVAILABLE" : "OFF");

    u8g2.setCursor(8, 56);
    u8g2.print("PCA : ");
    u8g2.print(gpio_expander::isReady() ? "READY" : "ERROR");
  }

  void drawWifiInfoScreen()
  {
    drawHeader("WIFI SERVER");

    u8g2.setFont(u8g2_font_5x8_tr);
    drawCenteredText(0, 24, 128, 10, "SSID: SHIOTEMS");
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
    SPI.begin(Pins::LCD_CLK, -1, Pins::LCD_MOSI, Pins::LCD_CS);
    u8g2.begin();
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
