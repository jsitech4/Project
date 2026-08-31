#include <Arduino.h>

#include "Pins.h"
#include "gpio_expander/gpio_expander.h"
#include "inverter_sense.h"

namespace inverter_sense
{
  // =========================================================
  // STATE
  // =========================================================

  bool available = false;
  bool raw = false;

  // =========================================================
  // ROLLING FILTER
  // =========================================================

  /*
   * We don't synchronize the sampling to the AC waveform.
   *
   * The filter looks at a large number of samples instead.
   */

  constexpr unsigned long SAMPLE_INTERVAL = 7;

  // Filter window = approximately 1 second
  constexpr uint16_t WINDOW_SAMPLES = 140;

  /*
   * ON threshold:
   *
   * At least 90% of samples must be HIGH.
   */
  constexpr uint16_t ON_PERCENT = 90;

  /*
   * OFF threshold:
   *
   * At least 90% of samples must be LOW.
   */
  constexpr uint16_t OFF_PERCENT = 90;

  uint16_t sampleCount = 0;
  uint16_t highCount = 0;

  unsigned long lastSampleTime = 0;

  // =========================================================
  // CONVERT PHYSICAL INPUT TO LOGICAL STATE
  // =========================================================

  bool convertToAvailable(bool physicalState)
  {
    if (Pins::INVERTER_SENSE_ACTIVE_HIGH)
    {
      return physicalState;
    }

    return !physicalState;
  }

  // =========================================================
  // RESET
  // =========================================================

  void resetFilter()
  {
    available = false;
    raw = false;

    sampleCount = 0;
    highCount = 0;

    lastSampleTime = millis();
  }

  // =========================================================
  // BEGIN
  // =========================================================

  void begin()
  {
    resetFilter();

    if (!gpio_expander::isReady())
    {
      return;
    }

    gpio_expander::pinMode(
        Pins::EXP_INVERTER_SENSE,
        INPUT);

    /*
     * Do NOT trust the initial state.
     *
     * The rolling filter must qualify it first.
     */
    raw = gpio_expander::digitalRead(
        Pins::EXP_INVERTER_SENSE);

    lastSampleTime = millis();
  }

  // =========================================================
  // UPDATE
  // =========================================================

  void update()
  {
    if (!gpio_expander::isReady())
    {
      resetFilter();
      return;
    }

    unsigned long now = millis();

    if ((unsigned long)(now - lastSampleTime) < SAMPLE_INTERVAL)
    {
      return;
    }

    lastSampleTime = now;

    // -----------------------------------------------------
    // READ PCA9555
    // -----------------------------------------------------

    raw = gpio_expander::digitalRead(
        Pins::EXP_INVERTER_SENSE);

    // -----------------------------------------------------
    // CONVERT TO LOGICAL STATE
    // -----------------------------------------------------

    bool sensedState = convertToAvailable(raw);

    // -----------------------------------------------------
    // ACCUMULATE SAMPLE
    // -----------------------------------------------------

    sampleCount++;

    if (sensedState)
    {
      highCount++;
    }

    // -----------------------------------------------------
    // WINDOW NOT COMPLETE
    // -----------------------------------------------------

    if (sampleCount < WINDOW_SAMPLES)
    {
      return;
    }

    // =====================================================
    // WINDOW COMPLETE
    // =====================================================

    uint16_t highPercentage =
        (uint32_t)highCount * 100UL / sampleCount;

    uint16_t lowPercentage =
        100 - highPercentage;

    // =====================================================
    // CURRENTLY OFF
    // =====================================================

    if (!available)
    {
      /*
       * Only turn ON if the signal was HIGH for at least
       * 90% of the entire window.
       */
      if (highPercentage >= ON_PERCENT)
      {
        available = true;
      }
    }

    // =====================================================
    // CURRENTLY ON
    // =====================================================

    else
    {
      /*
       * Only turn OFF if the signal was LOW for at least
       * 90% of the entire window.
       */
      if (lowPercentage >= OFF_PERCENT)
      {
        available = false;
      }
    }

    // -----------------------------------------------------
    // RESET WINDOW
    // -----------------------------------------------------

    sampleCount = 0;
    highCount = 0;
  }

  // =========================================================
  // CONFIRMED STATE
  // =========================================================

  bool isAvailable()
  {
    return available;
  }

  // =========================================================
  // RAW STATE
  // =========================================================

  bool rawState()
  {
    return raw;
  }
}
