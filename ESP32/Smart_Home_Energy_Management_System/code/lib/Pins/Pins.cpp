#include <Arduino.h>
#include "Pins.h"

namespace Pins
{
  const uint8_t HEARTBEAT_LED = 2;

  const uint8_t LCD_CS = 5;
  const uint8_t LCD_CLK = 18;
  const uint8_t LCD_MOSI = 23;

  const uint8_t I2C_SDA = 21;
  const uint8_t I2C_SCL = 22;

  const uint8_t PZEM_RX = 16;
  const uint8_t PZEM_TX = 17;

  const uint8_t ENCODER_A = 26;
  const uint8_t ENCODER_B = 25;
  const uint8_t ENCODER_SW = 27;

  const uint8_t PCA9555_ADDRESS = 0x20;

  // PCA9555/PCA9535 expander pin numbering used in this project:
  // P0_0..P0_7 => 0..7, P1_0..P1_7 => 8..15.
  // Correct load/source pair mapping supplied for the board:
  // Load 1: P1_2, Source 1: P1_3
  // Load 2: P1_0, Source 2: P1_1
  // Load 3: P0_6, Source 3: P0_7
  // Load 4: P0_4, Source 4: P0_5
  // Load 5: P0_2, Source 5: P0_3
  // Load 6: P0_0, Source 6: P0_1
  const uint8_t EXP_RELAY_1_LOAD = 10;    // P1_2
  const uint8_t EXP_RELAY_1_SOURCE = 11;  // P1_3
  const uint8_t EXP_RELAY_2_LOAD = 8;     // P1_0
  const uint8_t EXP_RELAY_2_SOURCE = 9;   // P1_1
  const uint8_t EXP_RELAY_3_LOAD = 6;     // P0_6
  const uint8_t EXP_RELAY_3_SOURCE = 7;   // P0_7
  const uint8_t EXP_RELAY_4_LOAD = 4;     // P0_4
  const uint8_t EXP_RELAY_4_SOURCE = 5;   // P0_5
  const uint8_t EXP_RELAY_5_LOAD = 2;     // P0_2
  const uint8_t EXP_RELAY_5_SOURCE = 3;   // P0_3
  const uint8_t EXP_RELAY_6_LOAD = 0;     // P0_0
  const uint8_t EXP_RELAY_6_SOURCE = 1;   // P0_1

  const uint8_t EXP_IND_LED = 12;         // P1_4 fault/status indicator
  const uint8_t EXP_12V_EN = 13;          // P1_5 regulator enable
  const uint8_t EXP_INVERTER_SENSE = 14;  // P1_6 inverter sense
  const uint8_t EXP_NEPA_SENSE = 15;      // P1_7 NEPA/PHCN sense

  const bool SOURCE_SENSE_ACTIVE_HIGH = true;
  const bool INVERTER_SENSE_ACTIVE_HIGH = true;

  void begin()
  {
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);
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
