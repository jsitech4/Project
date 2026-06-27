#include <Arduino.h>
#include <FS.h>
#include <SPI.h>
#include <SD.h>
#include <LittleFS.h>
#include "sd_card.h"
#include "Pins.h"

namespace sd_card
{
  static bool ready = false;
  static bool sdReady = false;
  static bool internalReady = false;
  static fs::FS *activeFs = nullptr;
  static String storageName = "None";

  static const char *configPath = "/config.txt";
  static const char *usersPath = "/users.csv";
  static const char *attendancePath = "/attendance.csv";

  static String trimValue(String value)
  {
    value.trim();
    return value;
  }

  static String cleanConfigField(String value)
  {
    value.trim();
    value.replace("\r", " ");
    value.replace("\n", " ");

    if (value.length() == 0)
      value = "-";

    return value;
  }

  static String csvPart(const String &line, uint8_t index)
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

  static fs::FS &fsRef()
  {
    return *activeFs;
  }

  static void prepareStorageBus()
  {
    if (Pins::valid(Pins::RC522_SS))
      digitalWrite(Pins::RC522_SS, HIGH);

    if (Pins::valid(Pins::SD_CARD_CS))
      digitalWrite(Pins::SD_CARD_CS, HIGH);
  }

  static void initializeFiles()
  {
    ensureFile(configPath,
               "system_name=Fingerprint RFID Attendance System\n"
               "workspace_name=Prototype Workspace\n"
               "workspace_type=school\n"
               "ap_ssid=AttendanceSystem\n"
               "ap_password=12345678\n"
               "default_mode=IN\n");

    ensureFile(usersPath, "user_id,name,workspace,rfid_uid,fingerprint_id,active,created_at\n");
    ensureFile(attendancePath, "timestamp,user_id,name,method,direction,workspace,credential,record_hash\n");
  }

  void begin()
  {
    ready = false;
    sdReady = false;
    internalReady = false;
    activeFs = nullptr;
    storageName = "None";

    SPI.begin(Pins::SHARED_SCK, Pins::SHARED_MISO, Pins::SHARED_MOSI);

    pinMode(Pins::RC522_SS, OUTPUT);
    digitalWrite(Pins::RC522_SS, HIGH);

    pinMode(Pins::SD_CARD_CS, OUTPUT);
    digitalWrite(Pins::SD_CARD_CS, HIGH);

    prepareStorageBus();
    sdReady = SD.begin(Pins::SD_CARD_CS, SPI, 4000000);

    if (sdReady)
    {
      activeFs = &SD;
      storageName = "SD Card";
      ready = true;
      initializeFiles();
      return;
    }

    prepareStorageBus();
    internalReady = LittleFS.begin(true);

    if (internalReady)
    {
      activeFs = &LittleFS;
      storageName = "Internal Flash";
      ready = true;
      initializeFiles();
      return;
    }

    ready = false;
  }

  void update()
  {
  }

  bool isReady()
  {
    return ready && activeFs != nullptr;
  }

  bool isSdCardReady()
  {
    return ready && sdReady && activeFs == &SD;
  }

  bool isInternalReady()
  {
    return ready && internalReady && activeFs == &LittleFS;
  }

  String getStorageName()
  {
    return storageName;
  }

  bool exists(const String &path)
  {
    if (!isReady())
      return false;

    prepareStorageBus();
    return fsRef().exists(path);
  }

  File openRead(const String &path)
  {
    if (!isReady())
      return File();

    prepareStorageBus();
    return fsRef().open(path, FILE_READ);
  }

  bool appendLine(const String &path, const String &line)
  {
    if (!isReady())
      return false;

    prepareStorageBus();
    File file = fsRef().open(path, FILE_APPEND);

    if (!file)
      return false;

    file.println(line);
    file.close();

    return true;
  }

  bool writeFile(const String &path, const String &content)
  {
    if (!isReady())
      return false;

    prepareStorageBus();

    if (fsRef().exists(path))
      fsRef().remove(path);

    File file = fsRef().open(path, FILE_WRITE);

    if (!file)
      return false;

    file.print(content);
    file.close();

    return true;
  }

  bool ensureFile(const String &path, const String &header)
  {
    if (!isReady())
      return false;

    prepareStorageBus();

    if (fsRef().exists(path))
      return true;

    File file = fsRef().open(path, FILE_WRITE);

    if (!file)
      return false;

    file.print(header);
    file.close();

    return true;
  }

  String readFile(const String &path, size_t maxBytes)
  {
    String data;

    if (!isReady())
      return data;

    prepareStorageBus();
    File file = fsRef().open(path, FILE_READ);

    if (!file)
      return data;

    while (file.available() && data.length() < maxBytes)
      data += (char)file.read();

    file.close();

    return data;
  }

  String getConfigValue(const String &key, const String &fallback)
  {
    if (!isReady())
      return fallback;

    prepareStorageBus();
    File file = fsRef().open(configPath, FILE_READ);

    if (!file)
      return fallback;

    while (file.available())
    {
      String line = file.readStringUntil('\n');
      line.trim();

      if (line.length() == 0 || line.startsWith("#"))
        continue;

      int split = line.indexOf('=');

      if (split <= 0)
        continue;

      String k = trimValue(line.substring(0, split));
      String v = trimValue(line.substring(split + 1));

      if (k == key)
      {
        file.close();
        return v.length() ? v : fallback;
      }
    }

    file.close();

    return fallback;
  }

  bool saveConfig(const String &systemName,
                  const String &workspaceName,
                  const String &workspaceType,
                  const String &apSsid,
                  const String &apPassword,
                  const String &defaultMode)
  {
    String mode = defaultMode;
    mode.toUpperCase();

    if (mode != "OUT")
      mode = "IN";

    String type = workspaceType;
    type.toLowerCase();

    if (type != "workspace")
      type = "school";

    String ssid = apSsid;
    ssid.trim();

    String pass = apPassword;
    pass.trim();

    if (ssid.length() < 1 || ssid.length() > 32)
      ssid = "AttendanceSystem";

    if (pass.length() < 8 || pass.length() > 63)
      pass = "12345678";

    String content;
    content += "system_name=" + cleanConfigField(systemName) + "\n";
    content += "workspace_name=" + cleanConfigField(workspaceName) + "\n";
    content += "workspace_type=" + type + "\n";
    content += "ap_ssid=" + ssid + "\n";
    content += "ap_password=" + pass + "\n";
    content += "default_mode=" + mode + "\n";

    return writeFile(configPath, content);
  }

  uint32_t countDataLines(const String &path)
  {
    if (!isReady())
      return 0;

    prepareStorageBus();
    File file = fsRef().open(path, FILE_READ);

    if (!file)
      return 0;

    uint32_t lines = 0;
    bool first = true;

    while (file.available())
    {
      String line = file.readStringUntil('\n');
      line.trim();

      if (line.length() == 0)
        continue;

      if (first)
      {
        first = false;
        continue;
      }

      lines++;
    }

    file.close();

    return lines;
  }

  bool clearAttendance()
  {
    return writeFile(attendancePath, "timestamp,user_id,name,method,direction,workspace,credential,record_hash\n");
  }

  uint16_t getUserFingerId(const String &userId)
  {
    if (!isReady())
      return 0;

    String cleanId = userId;
    cleanId.trim();

    if (cleanId.length() == 0)
      return 0;

    prepareStorageBus();
    File file = fsRef().open(usersPath, FILE_READ);

    if (!file)
      return 0;

    while (file.available())
    {
      String line = file.readStringUntil('\n');
      line.trim();

      if (line.length() == 0)
        continue;

      if (line.startsWith("user_id"))
        continue;

      String id = csvPart(line, 0);

      if (id == cleanId)
      {
        String fingerText = csvPart(line, 4);
        file.close();
        return fingerText.toInt();
      }
    }

    file.close();

    return 0;
  }

  bool deleteUser(const String &userId)
  {
    if (!isReady())
      return false;

    String cleanId = userId;
    cleanId.trim();

    if (cleanId.length() == 0)
      return false;

    prepareStorageBus();

    if (!fsRef().exists(usersPath))
      return false;

    File source = fsRef().open(usersPath, FILE_READ);

    if (!source)
      return false;

    if (fsRef().exists("/users_tmp.csv"))
      fsRef().remove("/users_tmp.csv");

    File temp = fsRef().open("/users_tmp.csv", FILE_WRITE);

    if (!temp)
    {
      source.close();
      return false;
    }

    bool deleted = false;

    while (source.available())
    {
      String line = source.readStringUntil('\n');
      line.trim();

      if (line.length() == 0)
        continue;

      if (line.startsWith("user_id"))
      {
        temp.println(line);
        continue;
      }

      String id = csvPart(line, 0);

      if (id == cleanId)
      {
        deleted = true;
        continue;
      }

      temp.println(line);
    }

    source.close();
    temp.close();

    if (!deleted)
    {
      fsRef().remove("/users_tmp.csv");
      return false;
    }

    fsRef().remove(usersPath);

    if (!fsRef().rename("/users_tmp.csv", usersPath))
      return false;

    return true;
  }

  uint64_t totalBytes()
  {
    if (isSdCardReady())
      return SD.totalBytes();

    if (isInternalReady())
      return LittleFS.totalBytes();

    return 0;
  }

  uint64_t usedBytes()
  {
    if (isSdCardReady())
      return SD.usedBytes();

    if (isInternalReady())
      return LittleFS.usedBytes();

    return 0;
  }
}
