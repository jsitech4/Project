#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

namespace Pins
{
  extern const uint8_t I2C_SDA;
  extern const uint8_t I2C_SCL;

  extern const uint8_t ADXL345_INT;
  extern const uint8_t ZMCT103C_ADC;
  extern const uint8_t DS18B20_DATA;

  extern const uint8_t SD_CS;
  extern const uint8_t SD_MOSI;
  extern const uint8_t SD_SCK;
  extern const uint8_t SD_MISO;

  extern const uint8_t BUZZER;

  extern const uint8_t ENCODER_CLK;
  extern const uint8_t ENCODER_DT;
  extern const uint8_t ENCODER_SW;

  extern const uint8_t RELAY;
  extern const uint8_t STATUS_LED;

  extern const uint8_t UART0_TX;
  extern const uint8_t UART0_RX;
  extern const uint8_t BOOT_PIN;

  void begin();

  int readPin(uint8_t gpio);
  void writePin(uint8_t gpio, bool value);
}

#endif
