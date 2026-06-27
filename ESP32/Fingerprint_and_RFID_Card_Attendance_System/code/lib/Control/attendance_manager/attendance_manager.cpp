#include <Arduino.h>
#include <FS.h>
#include "attendance_manager.h"
#include "RFID/RFID.h"
#include "fingerprint/fingerprint.h"
#include "sd_card/sd_card.h"
#include "rtc/rtc.h"
#include "oled_screen/oled_screen.h"
#include "buzzer/buzzer.h"
#include "buttons/buttons.h"
#include "mbedtls/sha256.h"

namespace attendance_manager
{
  struct UserRecord
  {
    String userId;
    String name;
    String workspace;
    String rfidUid;
    uint16_t fingerId;
    bool active;
    String createdAt;
  };

  enum AuthState
  {
    AUTH_IDLE,
    AUTH_WAIT_FINGER
  };

  static Mode currentMode = MODE_IN;
  static EnrollmentState enrollmentState = ENROLL_IDLE;
  static AuthState authState = AUTH_IDLE;

  static String pendingUserId;
  static String pendingName;
  static String pendingWorkspace;
  static String pendingRfid;
  static uint16_t pendingFingerId = 0;

  static UserRecord authUser;
  static String authRfid = "";
  static unsigned long authStartTime = 0;
  static unsigned long enrollmentStartTime = 0;

  static String lastMessage = "Starting";
  static uint32_t userCount = 0;
  static uint32_t attendanceCount = 0;

  static String lastCredential = "";
  static unsigned long lastLogTime = 0;

  static const unsigned long authTimeoutMs = 15000;
  static const unsigned long duplicateWindowMs = 5000;
  static const unsigned long enrollmentTimeoutMs = 90000;

  static String cleanField(String value)
  {
    value.trim();
    value.replace(",", " ");
    value.replace("\r", " ");
    value.replace("\n", " ");

    if (value.length() == 0)
      value = "-";

    return value;
  }

  static String csvAt(const String &line, uint8_t index)
  {
    int start = 0;
    uint8_t current = 0;

    for (uint16_t i = 0; i <= line.length(); i++)
    {
      if (i == line.length() || line[i] == ',')
      {
        if (current == index)
        {
          String part = line.substring(start, i);
          part.trim();
          return part;
        }

        start = i + 1;
        current++;
      }
    }

    return "";
  }

  static bool parseUserLine(const String &line, UserRecord &user)
  {
    if (line.startsWith("user_id") || line.length() < 5)
      return false;

    user.userId = csvAt(line, 0);
    user.name = csvAt(line, 1);
    user.workspace = csvAt(line, 2);
    user.rfidUid = csvAt(line, 3);
    user.fingerId = (uint16_t)csvAt(line, 4).toInt();
    user.active = csvAt(line, 5) != "0";
    user.createdAt = csvAt(line, 6);

    return user.userId.length() > 0;
  }

  static String sha256(const String &input)
  {
    byte hash[32];

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);
    mbedtls_sha256_update_ret(&ctx, (const unsigned char *)input.c_str(), input.length());
    mbedtls_sha256_finish_ret(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    const char hex[] = "0123456789abcdef";
    String out;
    out.reserve(64);

    for (uint8_t i = 0; i < 32; i++)
    {
      out += hex[(hash[i] >> 4) & 0x0F];
      out += hex[hash[i] & 0x0F];
    }

    return out;
  }

  static bool findUserByRfid(const String &rfidUid, UserRecord &found)
  {
    if (!sd_card::isReady())
      return false;

    File file = sd_card::openRead("/users.csv");

    if (!file)
      return false;

    while (file.available())
    {
      String line = file.readStringUntil('\n');
      line.trim();

      UserRecord user;

      if (parseUserLine(line, user) && user.active && user.rfidUid == rfidUid)
      {
        found = user;
        file.close();
        return true;
      }
    }

    file.close();
    return false;
  }

  static bool userIdExists(const String &userId)
  {
    if (!sd_card::isReady())
      return false;

    File file = sd_card::openRead("/users.csv");

    if (!file)
      return false;

    while (file.available())
    {
      String line = file.readStringUntil('\n');
      line.trim();

      UserRecord user;

      if (parseUserLine(line, user) && user.userId == userId)
      {
        file.close();
        return true;
      }
    }

    file.close();
    return false;
  }

  static uint16_t nextFingerId()
  {
    uint16_t highest = 0;

    if (!sd_card::isReady())
      return 1;

    File file = sd_card::openRead("/users.csv");

    if (!file)
      return 1;

    while (file.available())
    {
      String line = file.readStringUntil('\n');
      line.trim();

      UserRecord user;

      if (parseUserLine(line, user) && user.fingerId > highest)
        highest = user.fingerId;
    }

    file.close();

    if (highest >= 200)
      return 0;

    return highest + 1;
  }

  static bool saveUser()
  {
    String created = rtc::getTimestamp();

    String line = cleanField(pendingUserId) + "," +
                  cleanField(pendingName) + "," +
                  cleanField(pendingWorkspace) + "," +
                  cleanField(pendingRfid) + "," +
                  String(pendingFingerId) + ",1," +
                  cleanField(created);

    return sd_card::appendLine("/users.csv", line);
  }

  static void resetAuth()
  {
    authState = AUTH_IDLE;
    authRfid = "";
    authStartTime = 0;
  }

  static void logAttendance(const UserRecord &user, const String &rfidUid, uint16_t fingerId)
  {
    String direction = getModeText();
    String method = "RFID+FINGER";
    String credential = rfidUid + "|FP:" + String(fingerId);

    String duplicateKey = user.userId + ":" + direction;

    if (duplicateKey == lastCredential && millis() - lastLogTime < duplicateWindowMs)
      return;

    String timestamp = rtc::getTimestamp();

    String base = cleanField(timestamp) + "," +
                  cleanField(user.userId) + "," +
                  cleanField(user.name) + "," +
                  cleanField(method) + "," +
                  cleanField(direction) + "," +
                  cleanField(user.workspace) + "," +
                  cleanField(credential);

    String hash = sha256(base);
    String line = base + "," + hash;

    if (sd_card::appendLine("/attendance.csv", line))
    {
      attendanceCount++;
      lastCredential = duplicateKey;
      lastLogTime = millis();

      lastMessage = user.name + " " + direction + " verified";
      oled_screen::show("Attendance Saved", user.name, direction + " verified", "Card + Finger OK");
      buzzer::doubleBeep();
    }
    else
    {
      lastMessage = "Storage log failed";
      oled_screen::showError("Storage failed");
      buzzer::errorBeep();
    }
  }

  static void handleButtons()
  {
    if (buttons::wasPressed(buttons::BUTTON_1))
      setMode(MODE_IN);

    if (buttons::wasPressed(buttons::BUTTON_2))
      setMode(MODE_OUT);

    if (buttons::wasPressed(buttons::BUTTON_5))
      oled_screen::show("System Status", "Mode: " + getModeText(), "Users: " + String(userCount), "Logs: " + String(attendanceCount));
  }

  static void handleEnrollment()
  {
    if (enrollmentState == ENROLL_IDLE)
      return;

    if (millis() - enrollmentStartTime > enrollmentTimeoutMs)
    {
      lastMessage = "Enrollment timeout";
      oled_screen::show("Enroll Timeout", pendingName, "Start again", "Web dashboard ready");
      buzzer::errorBeep();
      enrollmentState = ENROLL_IDLE;
      pendingUserId = "";
      pendingName = "";
      pendingWorkspace = "";
      pendingRfid = "";
      pendingFingerId = 0;
      return;
    }

    if (enrollmentState == ENROLL_WAIT_RFID)
    {
      String uid;

      if (RFID::readCard(uid))
      {
        UserRecord existingUser;

        if (findUserByRfid(uid, existingUser))
        {
          lastMessage = "RFID already registered";
          oled_screen::show("RFID In Use", existingUser.name, "Use another card", "or delete old user");
          buzzer::errorBeep();
          enrollmentState = ENROLL_IDLE;
          return;
        }

        pendingRfid = uid;
        pendingFingerId = nextFingerId();

        if (pendingFingerId == 0)
        {
          lastMessage = "Fingerprint ID full";
          oled_screen::showError("FP storage full");
          buzzer::errorBeep();
          enrollmentState = ENROLL_IDLE;
          return;
        }

        enrollmentState = ENROLL_WAIT_FINGER;
        lastMessage = "RFID linked. Place finger";
        oled_screen::show("Register Finger", pendingName, "ID: " + String(pendingFingerId), "Place finger");
        buzzer::beep();
      }

      return;
    }

    if (enrollmentState == ENROLL_WAIT_FINGER)
    {
      String message;
      bool ok = fingerprint::enroll(pendingFingerId, message);

      if (!ok)
      {
        lastMessage = message;
        oled_screen::showError(message);
        buzzer::errorBeep();
        enrollmentState = ENROLL_IDLE;
        return;
      }

      enrollmentState = ENROLL_SAVING;
    }

    if (enrollmentState == ENROLL_SAVING)
    {
      if (saveUser())
      {
        userCount++;
        lastMessage = pendingName + " registered";
        oled_screen::show("User Registered", pendingName, "RFID + Fingerprint", "Saved to storage");
        buzzer::doubleBeep();
      }
      else
      {
        lastMessage = "User save failed";
        oled_screen::showError("User save failed");
        buzzer::errorBeep();
      }

      enrollmentState = ENROLL_IDLE;
    }
  }

  static void handleTwoWayAttendance()
  {
    if (enrollmentState != ENROLL_IDLE)
      return;

    if (authState == AUTH_IDLE)
    {
      String uid;

      if (!RFID::readCard(uid))
        return;

      UserRecord user;

      if (!findUserByRfid(uid, user))
      {
        lastMessage = "Unknown RFID card";
        oled_screen::show("Unknown Card", uid, "Not registered", "Use web enroll");
        buzzer::errorBeep();
        return;
      }

      authUser = user;
      authRfid = uid;
      authStartTime = millis();
      authState = AUTH_WAIT_FINGER;

      lastMessage = "Card accepted. Place finger";
      oled_screen::show("Card Accepted", user.name, "Place finger now", "Timeout: 15 sec");
      buzzer::beep();

      return;
    }

    if (authState == AUTH_WAIT_FINGER)
    {
      if (millis() - authStartTime > authTimeoutMs)
      {
        lastMessage = "Authentication timeout";
        oled_screen::show("Auth Timeout", "Card accepted", "Finger not scanned", "Try again");
        buzzer::errorBeep();
        resetAuth();
        return;
      }

      String uid;

      if (RFID::readCard(uid))
      {
        UserRecord user;

        if (findUserByRfid(uid, user))
        {
          authUser = user;
          authRfid = uid;
          authStartTime = millis();

          lastMessage = "New card accepted";
          oled_screen::show("Card Accepted", user.name, "Place finger now", "Timeout: 15 sec");
          buzzer::beep();
        }
        else
        {
          lastMessage = "Unknown RFID card";
          oled_screen::show("Unknown Card", uid, "Not registered", "Use web enroll");
          buzzer::errorBeep();
          resetAuth();
        }

        return;
      }

      uint16_t fingerId = 0;
      uint16_t confidence = 0;

      if (!fingerprint::search(fingerId, confidence))
        return;

      if (fingerId != authUser.fingerId)
      {
        lastMessage = "Fingerprint mismatch";
        oled_screen::show("Auth Failed", authUser.name, "Finger mismatch", "Try again");
        buzzer::errorBeep();
        resetAuth();
        return;
      }

      logAttendance(authUser, authRfid, fingerId);
      resetAuth();
    }
  }

  void begin()
  {
    String defaultMode = sd_card::getConfigValue("default_mode", "IN");
    defaultMode.toUpperCase();

    currentMode = defaultMode == "OUT" ? MODE_OUT : MODE_IN;

    refreshCounts();

    enrollmentState = ENROLL_IDLE;
    resetAuth();

    lastMessage = "Ready";
    oled_screen::show("Attendance Ready", "Tap card first", "Then fingerprint", "Mode: " + getModeText());
  }

  void update()
  {
    handleButtons();

    bool enrollmentWasBusy = enrollmentState != ENROLL_IDLE;

    handleEnrollment();

    if (enrollmentWasBusy)
      return;

    handleTwoWayAttendance();
  }

  bool startEnrollment(String userId, String name, String workspace)
  {
    if (enrollmentState != ENROLL_IDLE)
    {
      lastMessage = "Enrollment busy";
      return false;
    }

    if (authState != AUTH_IDLE)
      resetAuth();

    userId = cleanField(userId);
    name = cleanField(name);
    workspace = cleanField(workspace);

    if (userId == "-" || name == "-")
    {
      lastMessage = "Missing user details";
      return false;
    }

    if (!sd_card::isReady())
    {
      lastMessage = "Storage not ready";
      return false;
    }

    if (userIdExists(userId))
    {
      lastMessage = "User ID already exists";
      return false;
    }

    pendingUserId = userId;
    pendingName = name;
    pendingWorkspace = workspace;
    pendingRfid = "";
    pendingFingerId = 0;

    enrollmentState = ENROLL_WAIT_RFID;
    enrollmentStartTime = millis();

    lastMessage = "Tap RFID card for " + name;
    oled_screen::show("Register User", name, "Tap RFID card", "Then fingerprint");
    buzzer::beep();

    return true;
  }

  bool deleteUser(const String &userId)
  {
    if (enrollmentState != ENROLL_IDLE)
    {
      lastMessage = "Enrollment busy";
      return false;
    }

    resetAuth();

    String cleanId = cleanField(userId);

    if (cleanId == "-")
    {
      lastMessage = "Invalid user ID";
      return false;
    }

    if (!sd_card::isReady())
    {
      lastMessage = "Storage not ready";
      return false;
    }

    uint16_t fingerId = sd_card::getUserFingerId(cleanId);

    if (fingerId > 0)
    {
      String fpMessage;
      fingerprint::deleteTemplate(fingerId, fpMessage);
    }

    bool ok = sd_card::deleteUser(cleanId);

    if (!ok)
    {
      lastMessage = "User delete failed";
      oled_screen::showError("Delete failed");
      buzzer::errorBeep();
      return false;
    }

    refreshCounts();

    lastMessage = "User deleted";
    oled_screen::show("User Deleted", cleanId, "RFID unlinked", "Fingerprint removed");
    buzzer::doubleBeep();

    return true;
  }

  bool isEnrollmentBusy()
  {
    return enrollmentState != ENROLL_IDLE;
  }

  EnrollmentState getEnrollmentState()
  {
    return enrollmentState;
  }

  void setMode(Mode mode)
  {
    currentMode = mode;
    resetAuth();

    lastMessage = "Mode set to " + getModeText();
    oled_screen::show("Mode Changed", "Attendance mode", getModeText(), "Tap card first");
    buzzer::beep();
  }

  Mode getMode()
  {
    return currentMode;
  }

  String getModeText()
  {
    return currentMode == MODE_IN ? "IN" : "OUT";
  }

  String getLastMessage()
  {
    return lastMessage;
  }

  void refreshCounts()
  {
    userCount = sd_card::countDataLines("/users.csv");
    attendanceCount = sd_card::countDataLines("/attendance.csv");
  }

  uint32_t getUserCount()
  {
    return userCount;
  }

  uint32_t getAttendanceCount()
  {
    return attendanceCount;
  }
}
