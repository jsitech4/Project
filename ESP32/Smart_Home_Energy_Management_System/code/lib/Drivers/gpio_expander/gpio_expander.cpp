#include <Arduino.h>
#include <Wire.h>
#include "Pins.h"
#include "gpio_expander.h"

namespace gpio_expander
{
  const uint8_t REG_INPUT_0 = 0x00;
  const uint8_t REG_INPUT_1 = 0x01;
  const uint8_t REG_OUTPUT_0 = 0x02;
  const uint8_t REG_OUTPUT_1 = 0x03;
  const uint8_t REG_POLARITY_0 = 0x04;
  const uint8_t REG_POLARITY_1 = 0x05;
  const uint8_t REG_CONFIG_0 = 0x06;
  const uint8_t REG_CONFIG_1 = 0x07;

  uint8_t deviceAddress = Pins::PCA9555_ADDRESS;

  uint16_t outputState = 0x0000;
  uint16_t inputState = 0x0000;
  uint16_t configState = 0xFFFF;

  bool ready = false;

  bool writeRegister(uint8_t reg, uint8_t value)
  {
    Wire.beginTransmission(deviceAddress);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
  }

  uint8_t readRegister(uint8_t reg)
  {
    Wire.beginTransmission(deviceAddress);
    Wire.write(reg);

    if (Wire.endTransmission(false) != 0)
      return 0;

    Wire.requestFrom(deviceAddress, (uint8_t)1);

    if (Wire.available())
      return Wire.read();

    return 0;
  }

  bool writeWord(uint8_t lowReg, uint16_t value)
  {
    bool ok1 = writeRegister(lowReg, value & 0xFF);
    bool ok2 = writeRegister(lowReg + 1, (value >> 8) & 0xFF);
    return ok1 && ok2;
  }

  uint16_t readWord(uint8_t lowReg)
  {
    uint8_t low = readRegister(lowReg);
    uint8_t high = readRegister(lowReg + 1);
    return ((uint16_t)high << 8) | low;
  }

  bool testConnection()
  {
    Wire.beginTransmission(deviceAddress);
    return Wire.endTransmission() == 0;
  }

  void applyConfig()
  {
    if (!ready)
      return;

    writeWord(REG_CONFIG_0, configState);
  }

  void applyOutput()
  {
    if (!ready)
      return;

    writeWord(REG_OUTPUT_0, outputState);
  }

  void configureDefaultHardware()
  {
    writeWord(REG_POLARITY_0, 0x0000);

    configState = 0xC000;  // P1_6 and P1_7 are source-sense inputs; all relay/fault/regulator pins are outputs.
    outputState = 0x0000;

    applyOutput();
    applyConfig();
  }

  void begin()
  {
    begin(Pins::PCA9555_ADDRESS, Pins::I2C_SDA, Pins::I2C_SCL);
  }

  void begin(uint8_t address)
  {
    begin(address, Pins::I2C_SDA, Pins::I2C_SCL);
  }

  void begin(uint8_t address, int sdaPin, int sclPin)
  {
    deviceAddress = address;

    Wire.begin(sdaPin, sclPin);
    Wire.setClock(100000);

    ready = testConnection();

    if (!ready)
      return;

    configureDefaultHardware();
  }

  void update()
  {
    if (!ready)
      return;

    inputState = readWord(REG_INPUT_0);
  }

  bool isReady()
  {
    return ready;
  }

  void pinMode(uint8_t pin, uint8_t mode)
  {
    if (!ready)
      return;

    if (pin > 15)
      return;

    if (mode == OUTPUT)
      configState &= ~(1 << pin);
    else
      configState |= (1 << pin);

    applyConfig();
  }

  void digitalWrite(uint8_t pin, bool state)
  {
    if (!ready)
      return;

    if (pin > 15)
      return;

    if (state)
      outputState |= (1 << pin);
    else
      outputState &= ~(1 << pin);

    applyOutput();
  }

  bool digitalRead(uint8_t pin)
  {
    if (!ready)
      return false;

    if (pin > 15)
      return false;

    inputState = readWord(REG_INPUT_0);

    return (inputState & (1 << pin)) != 0;
  }

  void writePort(uint16_t value)
  {
    if (!ready)
      return;

    outputState = value;
    applyOutput();
  }

  uint16_t readPort()
  {
    if (!ready)
      return 0;

    inputState = readWord(REG_INPUT_0);
    return inputState;
  }

  void setAllInputs()
  {
    if (!ready)
      return;

    configState = 0xFFFF;
    applyConfig();
  }

  void setAllOutputs()
  {
    if (!ready)
      return;

    configState = 0x0000;
    applyConfig();
  }

  uint16_t getOutputState()
  {
    return outputState;
  }

  uint16_t getInputState()
  {
    return inputState;
  }

  uint16_t getConfigState()
  {
    return configState;
  }
}
