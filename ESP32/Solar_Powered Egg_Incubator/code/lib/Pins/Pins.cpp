#include "Pins.h"
namespace Pins
{
  const uint8_t BATTERY_LEVEL = 34, SOLAR_LEVEL = 35, ULTRASONIC_TRIG = 27, ULTRASONIC_ECHO = 26, ENCODER_A = 32, ENCODER_B = 33, ENCODER_SW = 25, BUZZER = 4, HEATER_RELAY = 16, SPINNER_RELAY = 17, HUMIDIFIER_RELAY = 18, LCD_CS = 13, LCD_CLK = 14, LCD_MOSI = 23, I2C_SDA = 21, I2C_SCL = 22;
  void begin()
  {
    pinMode(BATTERY_LEVEL, INPUT);
    pinMode(SOLAR_LEVEL, INPUT);
    pinMode(ULTRASONIC_TRIG, OUTPUT);
    pinMode(ULTRASONIC_ECHO, INPUT);
    pinMode(ENCODER_A, INPUT_PULLUP);
    pinMode(ENCODER_B, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    pinMode(BUZZER, OUTPUT);
    pinMode(HEATER_RELAY, OUTPUT);
    pinMode(SPINNER_RELAY, OUTPUT);
    pinMode(HUMIDIFIER_RELAY, OUTPUT);
    digitalWrite(ULTRASONIC_TRIG, LOW);
    digitalWrite(BUZZER, LOW);
    digitalWrite(HEATER_RELAY, HIGH);
    digitalWrite(SPINNER_RELAY, HIGH);
    digitalWrite(HUMIDIFIER_RELAY, HIGH);
  }
  int readPin(uint8_t gpio) { return digitalRead(gpio); }
  void writePin(uint8_t gpio, bool value) { digitalWrite(gpio, value ? HIGH : LOW); }
}
