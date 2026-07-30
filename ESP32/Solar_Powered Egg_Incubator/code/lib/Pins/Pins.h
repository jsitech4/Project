#ifndef PINS_H
#define PINS_H
#include <Arduino.h>
namespace Pins
{
  extern const uint8_t BATTERY_LEVEL, SOLAR_LEVEL, ULTRASONIC_TRIG, ULTRASONIC_ECHO, ENCODER_A, ENCODER_B, ENCODER_SW, BUZZER, HEATER_RELAY, SPINNER_RELAY, HUMIDIFIER_RELAY, LCD_CS, LCD_CLK, LCD_MOSI, I2C_SDA, I2C_SCL, LCD_PWR, LCD_LED;
  void begin();
  int readPin(uint8_t gpio);
  void writePin(uint8_t gpio, bool value);
}
#endif
