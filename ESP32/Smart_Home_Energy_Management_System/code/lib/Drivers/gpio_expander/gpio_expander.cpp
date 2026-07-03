#include <Arduino.h>
#include <Wire.h>
#include "Pins.h"
#include "gpio_expander.h"

namespace gpio_expander
{
  const uint8_t REG_INPUT_0 = 0x00;
  const uint8_t REG_OUTPUT_0 = 0x02;
  const uint8_t REG_POLARITY_0 = 0x04;
  const uint8_t REG_CONFIG_0 = 0x06;

  uint8_t deviceAddress = Pins::PCA9555_ADDRESS;

  uint16_t outputState = 0x0000;
  uint16_t inputState = 0x0000;
  uint16_t configState = 0xFFFF;

  bool ready = false;
  unsigned long lastInputRefresh = 0;
  const unsigned long inputRefreshIntervalMs = 20;

  bool writeWord(uint8_t lowReg, uint16_t value)
  {
    Wire.beginTransmission(deviceAddress);
    Wire.write(lowReg);
    Wire.write((uint8_t)(value & 0xFF));
    Wire.write((uint8_t)((value >> 8) & 0xFF));
    return Wire.endTransmission() == 0;
  }

  uint16_t readWord(uint8_t lowReg)
  {
    Wire.beginTransmission(deviceAddress);
    Wire.write(lowReg);

    if (Wire.endTransmission(false) != 0)
      return inputState;

    uint8_t count = Wire.requestFrom(deviceAddress, (uint8_t)2);

    if (count < 2)
      return inputState;

    uint8_t low = Wire.read();
    uint8_t high = Wire.read();

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

    configState = 0xC000; // P1_6 and P1_7 are source-sense inputs; all relay/fault/regulator pins are outputs.
    outputState = 0x0000;

    applyOutput();
    applyConfig();
    inputState = readWord(REG_INPUT_0);
    lastInputRefresh = millis();
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
    Wire.setClock(400000);
    Wire.setTimeOut(20);

    ready = testConnection();

    if (!ready)
      return;

    configureDefaultHardware();
  }

  void update()
  {
    if (!ready)
      return;

    unsigned long now = millis();

    if (now - lastInputRefresh < inputRefreshIntervalMs)
      return;

    inputState = readWord(REG_INPUT_0);
    lastInputRefresh = now;
  }

  bool isReady()
  {
    return ready;
  }

  void pinMode(uint8_t pin, uint8_t mode)
  {
    if (!ready || pin > 15)
      return;

    uint16_t newConfig = configState;

    if (mode == OUTPUT)
      newConfig &= ~(1 << pin);
    else
      newConfig |= (1 << pin);

    if (newConfig == configState)
      return;

    configState = newConfig;
    applyConfig();
  }

  void digitalWrite(uint8_t pin, bool state)
  {
    if (!ready || pin > 15)
      return;

    uint16_t newOutput = outputState;

    if (state)
      newOutput |= (1 << pin);
    else
      newOutput &= ~(1 << pin);

    if (newOutput == outputState)
      return;

    outputState = newOutput;
    applyOutput();
  }

  bool digitalRead(uint8_t pin)
  {
    if (!ready || pin > 15)
      return false;

    // Use the cached input refreshed by update(). This prevents repeated I2C reads
    // from every sensor module in the same loop pass.
    return (inputState & (1 << pin)) != 0;
  }

  void writePort(uint16_t value)
  {
    if (!ready)
      return;

    if (outputState == value)
      return;

    outputState = value;
    applyOutput();
  }

  uint16_t readPort()
  {
    if (!ready)
      return 0;

    inputState = readWord(REG_INPUT_0);
    lastInputRefresh = millis();
    return inputState;
  }

  void setAllInputs()
  {
    if (!ready)
      return;

    if (configState == 0xFFFF)
      return;

    configState = 0xFFFF;
    applyConfig();
  }

  void setAllOutputs()
  {
    if (!ready)
      return;

    if (configState == 0x0000)
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
