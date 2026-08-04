#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Pins.h"
#include "lcd_screen.h"
#include "temp_sensor/temp_sensor.h"
#include "current_sensor/current_sensor.h"
#include "vibration_sensor/vibration_sensor.h"
#include "load_relay/load_relay.h"
#include "sd_card/sd_card.h"
#include "rotary_encoder/rotary_encoder.h"
#include "maintenance_manager/maintenance_manager.h"
#include "local_server/local_server.h"

namespace lcd_screen
{
  static LiquidCrystal_I2C lcd(0x27, 20, 4);

  static unsigned long lastUpdate = 0;
  static unsigned long lastScreenChange = 0;

  static uint8_t screen = 0;
  static const uint8_t screenCount = 5;

  static void printFixed(uint8_t col, uint8_t row, const String &text)
  {
    lcd.setCursor(col, row);

    String out = text;

    while (out.length() < 20 - col)
      out += ' ';

    if (out.length() > 20 - col)
      out = out.substring(0, 20 - col);

    lcd.print(out);
  }

  static String forecastShort(float minutes)
  {
    if (minutes < 0.0f)
      return "No trend";

    if (minutes < 60.0f)
      return String(minutes, 0) + " min";

    return String(minutes / 60.0f, 1) + " hr";
  }

  static void drawScreen()
  {
    maintenance_manager::Snapshot snap = maintenance_manager::getSnapshot();

    if (screen == 0)
    {
      printFixed(0, 0, "   MOTOR LIVE DATA  ");
      printFixed(0, 1, String("Temp: ") + (temp_sensor::isValid() ? String(temp_sensor::getTemperatureC(), 1) + " C" : "N/A"));
      printFixed(0, 2, String("Curr: ") + String(current_sensor::getCurrentA(), 2) + " A");
      printFixed(0, 3, String("Vib : ") + String(vibration_sensor::getVibrationRMS(), 3) + " g");
    }
    else if (screen == 1)
    {
      printFixed(0, 0, "  PREDICTIVE STATUS ");
      printFixed(0, 1, String("Level: ") + maintenance_manager::getLevelText());
      printFixed(0, 2, String("Risk : ") + String(maintenance_manager::getRiskScore(), 1) + "%");
      printFixed(0, 3, String("Health: ") + String(maintenance_manager::getHealthScore(), 1) + "%");
    }
    else if (screen == 2)
    {
      printFixed(0, 0, "   FUTURE ANALYSIS  ");
      printFixed(0, 1, String("Worst: ") + maintenance_manager::getWorstMetric());
      printFixed(0, 2, String("Forecast: ") + forecastShort(maintenance_manager::getForecastMinutes()));
      printFixed(0, 3, String("Backend : ") + String(sd_card::getBackendName()));
    }
    else if (screen == 3)
    {
      printFixed(0, 0, "    SYSTEM STATUS   ");
      printFixed(0, 1, String("Relay: ") + (load_relay::isOn() ? "ON" : "OFF"));
      printFixed(0, 2, String("Fault: ") + (maintenance_manager::isFault() ? "YES" : "NO"));
      // printFixed(0, 2, String("SD:") + (sd_card::isSdReady() ? "READY" : "NO") + " INT:" + (sd_card::isInternalReady() ? "OK" : "NO"));
      printFixed(0, 3, String("IP: ") + local_server::getIp());
    }
    else
    {
      printFixed(0, 0, "    ACCELERATION g   ");
      printFixed(0, 1, String("X: ") + String(snap.xG, 2));
      printFixed(0, 2, String("Y: ") + String(snap.yG, 2));
      printFixed(0, 3, String("Z: ") + String(snap.zG, 2));
    }
  }

  void begin()
  {
    Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

    lcd.init();
    lcd.backlight();
    lcd.clear();

    printFixed(0, 0, "Predictive Maint.");
    printFixed(0, 1, "Industrial Motor");
    // printFixed(0, 2, "ESP32-S3 WROOM-1U");
    printFixed(0, 3, "Starting system...");
  }

  void update()
  {
    unsigned long now = millis();

    bool screenChanged = false;

    int delta = rotary_encoder::getDelta();

    if (delta > 0)
    {
      screen = (screen + 1) % screenCount;
      lastScreenChange = now;
      screenChanged = true;
    }
    else if (delta < 0)
    {
      screen = (screen + screenCount - 1) % screenCount;
      lastScreenChange = now;
      screenChanged = true;
    }

    if ((now - lastScreenChange) >= 4000)
    {
      screen = (screen + 1) % screenCount;
      lastScreenChange = now;
      screenChanged = true;
    }

    if (screenChanged)
    {
      lcd.clear();
      drawScreen();
      lastUpdate = now;
      return;
    }

    if ((now - lastUpdate) >= 500)
    {
      drawScreen();
      lastUpdate = now;
    }
  }

  void setScreen(uint8_t index)
  {
    if (index >= screenCount)
      return;

    screen = index;
    lastUpdate = 0;
  }

  uint8_t getScreen()
  {
    return screen;
  }
}
