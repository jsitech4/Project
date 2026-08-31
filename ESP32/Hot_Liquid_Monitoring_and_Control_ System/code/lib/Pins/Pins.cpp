#include <Arduino.h>
#include "Pins.h"

namespace Pins
{
  const uint8_t I2C_SDA = 8;
  const uint8_t I2C_SCL = 9;

  const uint8_t ADXL345_INT = 10;
  const uint8_t ZMCT103C_ADC = 1;
  const uint8_t DS18B20_DATA = 4;

  const uint8_t SD_CS = 5;
  const uint8_t SD_MOSI = 11;
  const uint8_t SD_SCK = 12;
  const uint8_t SD_MISO = 13;

  const uint8_t BUZZER = 14;

  const uint8_t ENCODER_CLK = 16;
  const uint8_t ENCODER_DT = 17;
  const uint8_t ENCODER_SW = 18;

  const uint8_t RELAY = 21;
  const uint8_t STATUS_LED = 48;

  const uint8_t UART0_TX = 43;
  const uint8_t UART0_RX = 44;
  const uint8_t BOOT_PIN = 0;

  void begin()
  {
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW);

    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);

    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);

    pinMode(ADXL345_INT, INPUT_PULLUP);
    pinMode(ZMCT103C_ADC, INPUT);
  }

  int readPin(uint8_t gpio)
  {
    return digitalRead(gpio);
  }

  void writePin(uint8_t gpio, bool value)
  {
    digitalWrite(gpio, value ? HIGH : LOW);
  }
}
