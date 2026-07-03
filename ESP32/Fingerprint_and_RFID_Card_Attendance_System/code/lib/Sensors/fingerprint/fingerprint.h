#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <Arduino.h>

namespace fingerprint
{
  enum EnrollStatus
  {
    ENROLL_IDLE,
    ENROLL_RUNNING,
    ENROLL_SUCCESS,
    ENROLL_FAILED
  };

  void begin();
  void update();

  bool isReady();

  bool search(uint16_t &fingerId, uint16_t &confidence);
  bool startEnroll(uint16_t fingerId, String &message);
  EnrollStatus updateEnroll(String &message);
  void cancelEnroll();
  bool isEnrollBusy();
  bool deleteTemplate(uint16_t fingerId, String &message);

  uint16_t getTemplateCount();
}

#endif
