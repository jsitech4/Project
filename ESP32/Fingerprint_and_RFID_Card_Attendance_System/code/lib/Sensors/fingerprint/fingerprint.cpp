#include <Arduino.h>
#include <Adafruit_Fingerprint.h>
#include "fingerprint.h"
#include "Pins.h"

namespace fingerprint
{
  static HardwareSerial fingerSerial(1);
  static Adafruit_Fingerprint sensor(&fingerSerial);

  static bool ready = false;
  static uint16_t templateCount = 0;

  enum EnrollStep
  {
    STEP_IDLE,
    STEP_WAIT_FIRST_IMAGE,
    STEP_CONVERT_FIRST,
    STEP_WAIT_REMOVE,
    STEP_AFTER_REMOVE_PAUSE,
    STEP_WAIT_SECOND_IMAGE,
    STEP_CONVERT_SECOND,
    STEP_CREATE_MODEL,
    STEP_STORE_MODEL
  };

  static EnrollStep enrollStep = STEP_IDLE;
  static uint16_t enrollFingerId = 0;
  static unsigned long enrollStepStart = 0;
  static unsigned long enrollPauseStart = 0;

  static const unsigned long firstScanTimeoutMs = 15000;
  static const unsigned long removeTimeoutMs = 8000;
  static const unsigned long secondScanTimeoutMs = 15000;
  static const unsigned long afterRemovePauseMs = 600;

  static String imageErrorMessage(uint8_t code)
  {
    if (code == FINGERPRINT_PACKETRECIEVEERR)
      return "Fingerprint comm error";

    if (code == FINGERPRINT_IMAGEFAIL)
      return "Fingerprint image error";

    return "Fingerprint scan error";
  }

  static void resetEnrollment()
  {
    enrollStep = STEP_IDLE;
    enrollFingerId = 0;
    enrollStepStart = 0;
    enrollPauseStart = 0;
  }

  static EnrollStatus failEnrollment(String &message, const String &reason)
  {
    message = reason;
    resetEnrollment();
    return ENROLL_FAILED;
  }

  void begin()
  {
    ready = false;
    templateCount = 0;
    resetEnrollment();

    fingerSerial.begin(
        57600,
        SERIAL_8N1,
        Pins::FINGERPRINT_RX,
        Pins::FINGERPRINT_TX);

    sensor.begin(57600);

    ready = sensor.verifyPassword();

    if (ready)
    {
      sensor.getTemplateCount();
      templateCount = sensor.templateCount;
    }
  }

  void update()
  {
  }

  bool isReady()
  {
    return ready;
  }

  bool search(uint16_t &fingerId, uint16_t &confidence)
  {
    fingerId = 0;
    confidence = 0;

    if (!ready || enrollStep != STEP_IDLE)
      return false;

    uint8_t p = sensor.getImage();

    if (p != FINGERPRINT_OK)
      return false;

    p = sensor.image2Tz();

    if (p != FINGERPRINT_OK)
      return false;

    p = sensor.fingerFastSearch();

    if (p != FINGERPRINT_OK)
      return false;

    fingerId = sensor.fingerID;
    confidence = sensor.confidence;

    return true;
  }

  bool startEnroll(uint16_t fingerId, String &message)
  {
    if (!ready)
    {
      message = "Fingerprint not ready";
      return false;
    }

    if (enrollStep != STEP_IDLE)
    {
      message = "Fingerprint busy";
      return false;
    }

    if (fingerId < 1 || fingerId > 200)
    {
      message = "Invalid finger ID";
      return false;
    }

    enrollFingerId = fingerId;
    enrollStep = STEP_WAIT_FIRST_IMAGE;
    enrollStepStart = millis();
    enrollPauseStart = 0;
    message = "Place finger";
    return true;
  }

  EnrollStatus updateEnroll(String &message)
  {
    if (enrollStep == STEP_IDLE)
    {
      message = "No enrollment";
      return ENROLL_IDLE;
    }

    if (!ready)
      return failEnrollment(message, "Fingerprint not ready");

    unsigned long now = millis();
    uint8_t p = FINGERPRINT_NOFINGER;

    switch (enrollStep)
    {
    case STEP_WAIT_FIRST_IMAGE:
      if (now - enrollStepStart >= firstScanTimeoutMs)
        return failEnrollment(message, "Fingerprint timeout");

      p = sensor.getImage();

      if (p == FINGERPRINT_OK)
      {
        enrollStep = STEP_CONVERT_FIRST;
        message = "Processing first";
        return ENROLL_RUNNING;
      }

      if (p == FINGERPRINT_PACKETRECIEVEERR || p == FINGERPRINT_IMAGEFAIL)
        return failEnrollment(message, imageErrorMessage(p));

      message = "Place finger";
      return ENROLL_RUNNING;

    case STEP_CONVERT_FIRST:
      p = sensor.image2Tz(1);

      if (p != FINGERPRINT_OK)
        return failEnrollment(message, "First scan failed");

      enrollStep = STEP_WAIT_REMOVE;
      enrollStepStart = now;
      message = "Remove finger";
      return ENROLL_RUNNING;

    case STEP_WAIT_REMOVE:
      p = sensor.getImage();

      if (p == FINGERPRINT_NOFINGER || now - enrollStepStart >= removeTimeoutMs)
      {
        enrollStep = STEP_AFTER_REMOVE_PAUSE;
        enrollPauseStart = now;
        message = "Place same finger";
        return ENROLL_RUNNING;
      }

      if (p == FINGERPRINT_PACKETRECIEVEERR)
        return failEnrollment(message, "Fingerprint comm error");

      message = "Remove finger";
      return ENROLL_RUNNING;

    case STEP_AFTER_REMOVE_PAUSE:
      if (now - enrollPauseStart < afterRemovePauseMs)
      {
        message = "Place same finger";
        return ENROLL_RUNNING;
      }

      enrollStep = STEP_WAIT_SECOND_IMAGE;
      enrollStepStart = now;
      message = "Place same finger";
      return ENROLL_RUNNING;

    case STEP_WAIT_SECOND_IMAGE:
      if (now - enrollStepStart >= secondScanTimeoutMs)
        return failEnrollment(message, "Fingerprint timeout");

      p = sensor.getImage();

      if (p == FINGERPRINT_OK)
      {
        enrollStep = STEP_CONVERT_SECOND;
        message = "Processing second";
        return ENROLL_RUNNING;
      }

      if (p == FINGERPRINT_PACKETRECIEVEERR || p == FINGERPRINT_IMAGEFAIL)
        return failEnrollment(message, imageErrorMessage(p));

      message = "Place same finger";
      return ENROLL_RUNNING;

    case STEP_CONVERT_SECOND:
      p = sensor.image2Tz(2);

      if (p != FINGERPRINT_OK)
        return failEnrollment(message, "Second scan failed");

      enrollStep = STEP_CREATE_MODEL;
      message = "Matching scans";
      return ENROLL_RUNNING;

    case STEP_CREATE_MODEL:
      p = sensor.createModel();

      if (p != FINGERPRINT_OK)
        return failEnrollment(message, "Finger mismatch");

      enrollStep = STEP_STORE_MODEL;
      message = "Saving template";
      return ENROLL_RUNNING;

    case STEP_STORE_MODEL:
      p = sensor.storeModel(enrollFingerId);

      if (p != FINGERPRINT_OK)
        return failEnrollment(message, "Template save failed");

      sensor.getTemplateCount();
      templateCount = sensor.templateCount;
      resetEnrollment();
      message = "Fingerprint saved";
      return ENROLL_SUCCESS;

    default:
      return failEnrollment(message, "Enrollment error");
    }
  }

  void cancelEnroll()
  {
    resetEnrollment();
  }

  bool isEnrollBusy()
  {
    return enrollStep != STEP_IDLE;
  }

  bool deleteTemplate(uint16_t fingerId, String &message)
  {
    if (!ready)
    {
      message = "Fingerprint not ready";
      return false;
    }

    if (fingerId < 1 || fingerId > 200)
    {
      message = "Invalid finger ID";
      return false;
    }

    uint8_t p = sensor.deleteModel(fingerId);

    if (p != FINGERPRINT_OK)
    {
      message = "Fingerprint delete failed";
      return false;
    }

    sensor.getTemplateCount();
    templateCount = sensor.templateCount;

    message = "Fingerprint deleted";
    return true;
  }

  uint16_t getTemplateCount()
  {
    return templateCount;
  }
}
