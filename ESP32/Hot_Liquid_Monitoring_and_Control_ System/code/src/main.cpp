#include <Arduino.h>
#include <Wire.h>
#include "Pins.h"
#include "temp_sensor/temp_sensor.h"
#include "vibration_sensor/vibration_sensor.h"
#include "current_sensor/current_sensor.h"
#include "rotary_encoder/rotary_encoder.h"
#include "buzzer/buzzer.h"
#include "load_relay/load_relay.h"
#include "lcd_screen/lcd_screen.h"
#include "led_indicator/led_indicator.h"
#include "sd_card/sd_card.h"
#include "error_handling/error_handling.h"
#include "sleep_wake/sleep_wake.h"
#include "reset/reset.h"
#include "maintenance_manager/maintenance_manager.h"
#include "local_server/local_server.h"

static unsigned long lastSerialReport = 0;
static const unsigned long SERIAL_REPORT_INTERVAL_MS = 3000;

static void printSerialReport()
{
  unsigned long now = millis();

  if (now - lastSerialReport < SERIAL_REPORT_INTERVAL_MS)
  {
    return;
  }

  lastSerialReport = now;

  maintenance_manager::Snapshot snap = maintenance_manager::getSnapshot();

  Serial.println();
  // Serial.println("========== MOTOR PM STATUS ==========");
  // Serial.print("Uptime: ");
  // Serial.print(now / 1000);
  // Serial.println(" s");

  // Serial.print("Temp: ");
  if (snap.tempValid)
  {
    // Serial.print(snap.temperatureC, 2);
    // Serial.println(" C");
  }
  else
  {
    // Serial.println("N/A");
  }

  // Serial.print("Current: ");
  // Serial.print(snap.currentA, 3);
  // Serial.println(" A");

  // Serial.print("Vibration RMS: ");
  // Serial.print(snap.vibrationRmsG, 3);
  // Serial.println(" g");

  // Serial.print("Risk: ");
  // Serial.print(snap.riskScore, 1);
  // Serial.println(" %");

  // Serial.print("Health: ");
  // Serial.print(snap.healthScore, 1);
  // Serial.println(" %");

  // Serial.print("Level: ");
  // Serial.println(maintenance_manager::getLevelText());

  // Serial.print("Relay: ");
  // Serial.println(load_relay::isOn() ? "ON" : "OFF");

  // Serial.print("Storage Backend: ");
  // Serial.println(sd_card::getBackendName());

  // Serial.print("SD Ready: ");
  // Serial.println(sd_card::isSdReady() ? "YES" : "NO");

  // Serial.print("Internal Ready: ");
  // Serial.println(sd_card::isInternalReady() ? "YES" : "NO");

  // Serial.print("Dashboard: http://");
  // Serial.println(local_server::getIp());
  // Serial.println("=====================================");
}

void setup()
{
  Serial.begin(115200);
  delay(300);

  // Serial.println();
  // Serial.println("Booting Industrial Motor Predictive Maintenance System...");

  Pins::begin();
  Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

  error_handling::begin();
  sleep_wake::begin();
  reset::begin();

  rotary_encoder::begin();
  buzzer::begin();
  load_relay::begin();
  led_indicator::begin();

  temp_sensor::begin();
  vibration_sensor::begin();
  current_sensor::begin();

  maintenance_manager::begin();
  sd_card::begin();
  local_server::begin();
  lcd_screen::begin();

  sd_card::logEvent("BOOT", "Predictive maintenance firmware started.");
  buzzer::beep(120);

  // Serial.println("System started successfully.");
  // Serial.print("Dashboard SSID: ");
  // Serial.println(local_server::getSsid());
  // Serial.print("Dashboard IP: http://");
  // Serial.println(local_server::getIp());
  // Serial.print("Storage Backend: ");
  // Serial.println(sd_card::getBackendName());
}

void loop()
{
  rotary_encoder::update();

  temp_sensor::update();

  current_sensor::update();
  vibration_sensor::update();

  maintenance_manager::update();
  load_relay::update();

  buzzer::update();
  led_indicator::update();
  // buzzer::startAlarm();
  lcd_screen::update();

  sd_card::update();
  local_server::update();

  error_handling::update();
  sleep_wake::update();
  reset::update();

  // printSerialReport();

  yield();
}
