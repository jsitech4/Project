#include <Arduino.h>
#include <Wire.h>
#include "Pins.h"
#include "battery_level/battery_level.h"
#include "solar_level/solar_level.h"
#include "ultrasonic/ultrasonic.h"
#include "rotary_encoder/rotary_encoder.h"
#include "buzzer/buzzer.h"
#include "heater/heater.h"
#include "spinner/spinner.h"
#include "humidifier/humidifier.h"
#include "temp_hum/temp_hum.h"
#include "rtc_clock/rtc_clock.h"
#include "lcd_screen/lcd_screen.h"
#include "wifi_manager/wifi_manager.h"
#include "web_dashboard/web_dashboard.h"
#include "sleep_wake/sleep_wake.h"
#include "error_handling/error_handling.h"
#include "reset/reset.h"

static unsigned long lastPrint = 0;

void setup()
{
  Serial.begin(115200);
  Pins::begin();
  Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);
  battery_level::begin(Pins::BATTERY_LEVEL, 2.0f);
  solar_level::begin(Pins::SOLAR_LEVEL, 4.0f);
  ultrasonic::begin(Pins::ULTRASONIC_TRIG, Pins::ULTRASONIC_ECHO);
  rotary_encoder::begin(Pins::ENCODER_A, Pins::ENCODER_B, Pins::ENCODER_SW);
  buzzer::begin(Pins::BUZZER);
  heater::begin(Pins::HEATER_RELAY, true);
  spinner::begin(Pins::SPINNER_RELAY, true);
  humidifier::begin(Pins::HUMIDIFIER_RELAY, true);
  temp_hum::begin();
  rtc_clock::begin();
  lcd_screen::begin();
  wifi_manager::begin();
  web_dashboard::begin();
  sleep_wake::begin();
  error_handling::begin();
  reset::begin();
  Serial.println("Solar Powered Egg Incubator Ready");
  Serial.println("Connect to WiFi AP: Egg_Incubator");
  Serial.println("Password: 12345678");
  Serial.println("Dashboard: http://192.168.4.1");
}

void loop()
{
  battery_level::update();
  solar_level::update();
  ultrasonic::update();
  rotary_encoder::update();
  buzzer::update();
  heater::update();
  spinner::update();
  humidifier::update();
  temp_hum::update();
  rtc_clock::update();
  lcd_screen::update();
  wifi_manager::update();
  web_dashboard::update();
  sleep_wake::update();
  error_handling::update();
  reset::update();

  if (rotary_encoder::wasPressed())
    buzzer::beep(60, 60, 1);
}
