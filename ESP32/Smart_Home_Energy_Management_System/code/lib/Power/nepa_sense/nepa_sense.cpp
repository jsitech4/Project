// #include <Arduino.h>
// #include "Pins.h"
// #include "gpio_expander/gpio_expander.h"
// #include "nepa_sense.h"

// namespace nepa_sense
// {
//   bool available = false;
//   bool raw = false;

//   bool candidateState = false;

//   unsigned long candidateStartTime = 0;

//   const unsigned long AVAILABLE_DELAY = 0;
//   const unsigned long LOST_DELAY = 0;

//   void begin()
//   {
//     if (!gpio_expander::isReady())
//     {
//       available = false;
//       raw = false;
//       candidateState = false;
//       candidateStartTime = 0;
//       return;
//     }

//     gpio_expander::pinMode(
//         Pins::EXP_NEPA_SENSE,
//         INPUT);

//     raw = gpio_expander::digitalRead(
//         Pins::EXP_NEPA_SENSE);

//     if (Pins::SOURCE_SENSE_ACTIVE_HIGH)
//       available = raw;
//     else
//       available = !raw;

//     candidateState = available;
//     candidateStartTime = millis();
//   }

//   void update()
//   {
//     if (!gpio_expander::isReady())
//     {
//       available = false;
//       raw = false;
//       candidateState = false;
//       candidateStartTime = 0;
//       return;
//     }

//     raw = gpio_expander::digitalRead(
//         Pins::EXP_NEPA_SENSE);

//     bool sensedAvailable;

//     if (Pins::SOURCE_SENSE_ACTIVE_HIGH)
//       sensedAvailable = raw;
//     else
//       sensedAvailable = !raw;

//     unsigned long now = millis();

//     if (sensedAvailable == available)
//     {
//       candidateState = available;
//       candidateStartTime = now;
//       return;
//     }

//     if (sensedAvailable != candidateState)
//     {
//       candidateState = sensedAvailable;
//       candidateStartTime = now;
//       return;
//     }

//     unsigned long requiredTime;

//     if (candidateState)
//       requiredTime = AVAILABLE_DELAY;
//     else
//       requiredTime = LOST_DELAY;

//     if (now - candidateStartTime >= requiredTime)
//     {
//       available = candidateState;
//       candidateStartTime = now;
//     }
//   }

//   bool isAvailable()
//   {
//     return available;
//   }

//   bool rawState()
//   {
//     return raw;
//   }
// }


#include <Arduino.h>

#include "Pins.h"
#include "gpio_expander/gpio_expander.h"
#include "nepa_sense.h"

    namespace nepa_sense
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
   * 7 ms is intentionally not synchronized with 50/60 Hz
   * mains or the 100/120 Hz rectified waveform.
   */
  constexpr unsigned long SAMPLE_INTERVAL = 7;

  // Approximately 1 second of samples
  constexpr uint16_t WINDOW_SAMPLES = 140;

  /*
   * Source must be HIGH for >= 90% of the window to turn ON.
   */
  constexpr uint16_t ON_PERCENT = 90;

  /*
   * Source must be LOW for >= 90% of the window to turn OFF.
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
    if (Pins::SOURCE_SENSE_ACTIVE_HIGH)
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
        Pins::EXP_NEPA_SENSE,
        INPUT);

    /*
     * Read the input but don't immediately accept it.
     */
    raw = gpio_expander::digitalRead(
        Pins::EXP_NEPA_SENSE);

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
        Pins::EXP_NEPA_SENSE);

    // -----------------------------------------------------
    // CONVERT PHYSICAL INPUT
    // -----------------------------------------------------

    bool sensedState = convertToAvailable(raw);

    // -----------------------------------------------------
    // ACCUMULATE
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
    // CALCULATE DUTY RATIO
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
       * Require at least 90% HIGH.
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
       * Require at least 90% LOW.
       */
      if (lowPercentage >= OFF_PERCENT)
      {
        available = false;
      }
    }

    // -----------------------------------------------------
    // START NEW WINDOW
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

