#include <Arduino.h>
#include "Pins.h"
#include "oled_screen/oled_screen.h"
#include "buzzer/buzzer.h"
#include "rtc/rtc.h"
#include "sd_card/sd_card.h"
#include "RFID/RFID.h"
#include "fingerprint/fingerprint.h"
#include "buttons/buttons.h"
#include "battery_level/battery_level.h"
#include "led_indicator/led_indicator.h"
#include "error_handling/error_handling.h"
#include "sleep_wake/sleep_wake.h"
#include "reset/reset.h"
#include "wifi_manager/wifi_manager.h"
#include "local_server/local_server.h"
#include "attendance_manager/attendance_manager.h"
#include "keyboard_input/keyboard_input.h"

static bool lowBatteryShutdownPending = false;
static unsigned long lowBatteryShutdownAt = 0;

void setup()
{
  Serial.begin(115200);
  yield();

  Pins::begin();

  oled_screen::begin();
  oled_screen::showBoot();

  buzzer::begin(Pins::BUZZER_PIN);
  led_indicator::begin(Pins::STATUS_LED);

  error_handling::begin();
  sleep_wake::begin();
  reset::begin();

  rtc::begin();
  sd_card::begin();

  buttons::begin();
  battery_level::begin();

  RFID::begin();
  fingerprint::begin();

  wifi_manager::begin();
  attendance_manager::begin();
  local_server::begin();
  keyboard_input::begin();

  oled_screen::show("System Ready",
                    "Tap card first",
                    "Then fingerprint",
                    "Web: " + wifi_manager::getIpString(),
                    2500);
}

void loop()
{
  unsigned long now = millis();

  buttons::update();
  buzzer::update();
  rtc::update();
  battery_level::update();
  RFID::update();
  fingerprint::update();
  wifi_manager::update();
  local_server::update();
  keyboard_input::update();

  error_handling::setBatteryError(battery_level::isLow());

  if (battery_level::shouldSleep() && !lowBatteryShutdownPending)
  {
    lowBatteryShutdownPending = true;
    lowBatteryShutdownAt = now + 1500;
    oled_screen::showError("Low Battery");
  }

  if (lowBatteryShutdownPending && (long)(now - lowBatteryShutdownAt) >= 0)
    battery_level::sleepNow();

  attendance_manager::update();

  if (error_handling::hasError())
    led_indicator::update("fault");
  else if (attendance_manager::isEnrollmentBusy())
    led_indicator::update("active");
  else
    led_indicator::update();

  oled_screen::update();
  sd_card::update();
  sleep_wake::update();
  error_handling::update();
  reset::update();
}
