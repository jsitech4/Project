#include <Arduino.h>
#include "error_handling.h"
#include "Pins.h"
#include "gpio_expander/gpio_expander.h"
#include "pzem_sensor/pzem_sensor.h"
#include "nepa_sense/nepa_sense.h"
#include "inverter_sense/inverter_sense.h"

namespace error_handling
{
  ErrorCode currentError = NO_ERROR;

  void applyFaultIndicator()
  {
    if (!gpio_expander::isReady())
      return;

    gpio_expander::pinMode(Pins::EXP_IND_LED, OUTPUT);
    gpio_expander::digitalWrite(Pins::EXP_IND_LED, currentError != NO_ERROR);
  }

  void begin()
  {
    currentError = NO_ERROR;
    applyFaultIndicator();
  }

  void update()
  {
    if (!gpio_expander::isReady())
    {
      currentError = PCA9555_ERROR;
      return;
    }

    if (!nepa_sense::isAvailable() && !inverter_sense::isAvailable())
    {
      currentError = NO_SOURCE_ERROR;
      applyFaultIndicator();
      return;
    }

    if (!pzem_sensor::hasValidData())
    {
      currentError = PZEM_ERROR;
      applyFaultIndicator();
      return;
    }

    currentError = NO_ERROR;
    applyFaultIndicator();
  }

  void setError(ErrorCode error)
  {
    currentError = error;
    applyFaultIndicator();
  }

  void clearError()
  {
    currentError = NO_ERROR;
    applyFaultIndicator();
  }

  bool hasError()
  {
    return currentError != NO_ERROR;
  }

  ErrorCode getError()
  {
    return currentError;
  }
}
