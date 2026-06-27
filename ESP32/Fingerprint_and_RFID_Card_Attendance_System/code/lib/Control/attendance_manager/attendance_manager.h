#ifndef ATTENDANCE_MANAGER_H
#define ATTENDANCE_MANAGER_H

#include <Arduino.h>

namespace attendance_manager
{
  enum Mode
  {
    MODE_IN,
    MODE_OUT
  };

  enum EnrollmentState
  {
    ENROLL_IDLE,
    ENROLL_WAIT_RFID,
    ENROLL_WAIT_FINGER,
    ENROLL_SAVING
  };

  void begin();
  void update();

  bool startEnrollment(String userId, String name, String workspace);
  bool deleteUser(const String &userId);

  bool isEnrollmentBusy();
  EnrollmentState getEnrollmentState();

  void setMode(Mode mode);
  Mode getMode();
  String getModeText();

  String getLastMessage();

  uint32_t getUserCount();
  uint32_t getAttendanceCount();
  void refreshCounts();
}

#endif
