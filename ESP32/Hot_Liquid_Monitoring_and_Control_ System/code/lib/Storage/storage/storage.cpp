#include <Arduino.h>
#include <LittleFS.h>
#include <FS.h>
#include "storage.h"
#include "temp_sensor/temp_sensor.h"
#include "ultrasonic_sensor/ultrasonic_sensor.h"
#include "load_relay/load_relay.h"
#include "maintenance_manager/maintenance_manager.h"

namespace storage
{
  static fs::FS *activeFS = nullptr;
  static bool ready = false;
  static bool internalReady = false;

  static unsigned long lastLog = 0;
  static unsigned long logInterval = 5000;

  static const char *liquidLogFile = "/liquid_log.csv";
  static const char *analysisLogFile = "/analysis_log.csv";
  static const char *eventLogFile = "/event_log.csv";
  static const char *tankSettingsFile = "/tank_settings.cfg";

  static const char *backendName = "INTERNAL FLASH";

  static bool hasActiveStorage()
  {
    return ready && activeFS != nullptr;
  }

  static bool fileExists(const char *path)
  {
    if (!hasActiveStorage())
      return false;

    return activeFS->exists(path);
  }

  static String cleanCsvText(String text)
  {
    text.replace("\r", " ");
    text.replace("\n", " ");
    text.replace(",", ";");

    return text;
  }

  static bool appendLine(
      const char *path,
      const String &line)
  {
    if (!hasActiveStorage())
      return false;

    File file = activeFS->open(path, FILE_APPEND);

    if (!file)
    {
      file = activeFS->open(path, FILE_WRITE);
    }

    if (!file)
      return false;

    file.println(line);
    file.close();

    return true;
  }

  static void createFileWithHeader(
      const char *path,
      const String &header)
  {
    if (!hasActiveStorage())
      return;

    if (fileExists(path))
      return;

    File file = activeFS->open(path, FILE_WRITE);

    if (!file)
      return;

    file.println(header);
    file.close();
  }

  static void createHeaders()
  {
    createFileWithHeader(
        liquidLogFile,
        "millis,"
        "temp_c,"
        "temp_valid,"
        "distance_cm,"
        "level_percent,"
        "relay_on,"
        "relay_requested");

    createFileWithHeader(
        analysisLogFile,
        "millis,"
        "temperature_c,"
        "level_percent,"
        "distance_cm");

    createFileWithHeader(
        eventLogFile,
        "millis,"
        "event,"
        "message");
  }

  void begin()
  {
    ready = false;
    internalReady = false;
    activeFS = nullptr;

    internalReady = LittleFS.begin(true);

    if (!internalReady)
    {
      return;
    }

    activeFS = &LittleFS;
    ready = true;

    createHeaders();

    logEvent(
        "BOOT",
        "Internal flash storage initialized");
  }

  void update()
  {
    if (!ready)
      return;

    unsigned long now = millis();

    if (now - lastLog >= logInterval)
    {
      lastLog = now;
      logNow();
    }
  }

  void logNow()
  {
    if (!ready)
      return;

    maintenance_manager::Snapshot snap =
        maintenance_manager::getSnapshot();

    String liquidLine;

    liquidLine += String(millis());
    liquidLine += ",";

    // Temperature
    if (temp_sensor::isValid())
    {
      liquidLine += String(
          temp_sensor::getTemperatureC(),
          2);
    }
    else
    {
      liquidLine += "nan";
    }

    liquidLine += ",";

    // Temperature validity
    liquidLine +=
        temp_sensor::isValid() ? "1" : "0";

    liquidLine += ",";

    liquidLine += String(
        ultrasonic_sensor::getDistanceCm(),
        3);

    liquidLine += ",";

    liquidLine += String(
        snap.levelPercent,
        1);

    liquidLine += ",";

    liquidLine +=
        load_relay::isOn() ? "1" : "0";

    liquidLine += ",";

    liquidLine +=
        load_relay::getRequestedState() ? "1" : "0";

    appendLine(
        liquidLogFile,
        liquidLine);

    String analysisLine;

    analysisLine += String(millis());
    analysisLine += ",";

    analysisLine += String(snap.temperatureC, 1);
    analysisLine += ",";
    analysisLine += String(snap.levelPercent, 1);
    analysisLine += ",";
    analysisLine += String(snap.distanceCm, 1);

    appendLine(
        analysisLogFile,
        analysisLine);
  }

  void logEvent(
      const String &event,
      const String &message)
  {
    if (!ready)
      return;

    String line;

    line += String(millis());
    line += ",";

    line += cleanCsvText(event);
    line += ",";

    line += cleanCsvText(message);

    appendLine(
        eventLogFile,
        line);
  }

  bool isReady()
  {
    return ready;
  }

  bool isSdReady()
  {
    return false;
  }

  bool isInternalReady()
  {
    return internalReady;
  }

  String getBackendName()
  {
    return String(backendName);
  }

  void setLogInterval(unsigned long intervalMs)
  {
    if (intervalMs >= 1000)
    {
      logInterval = intervalMs;
    }
  }

  const char *getFileName()
  {
    return liquidLogFile;
  }

  const char *getLiquidLogFileName()
  {
    return liquidLogFile;
  }

  const char *getAnalysisLogFileName()
  {
    return analysisLogFile;
  }

  const char *getEventLogFileName()
  {
    return eventLogFile;
  }

  String readFile(const char *path)
  {
    if (!hasActiveStorage())
      return "";

    File file = activeFS->open(
        path,
        FILE_READ);

    if (!file)
      return "";

    String content;

    size_t count = 0;

    while (file.available())
    {
      content += char(file.read());

      count++;

      if ((count % 256) == 0)
      {
        yield();
      }
    }

    file.close();

    return content;
  }

  String readTail(
      const char *path,
      size_t maxBytes)
  {
    if (!hasActiveStorage())
      return "";

    File file = activeFS->open(
        path,
        FILE_READ);

    if (!file)
      return "";

    size_t size = file.size();

    if (size > maxBytes)
    {
      file.seek(size - maxBytes);
    }

    String content;

    size_t count = 0;

    while (file.available())
    {
      content += char(file.read());

      count++;

      if ((count % 256) == 0)
      {
        yield();
      }
    }

    file.close();

    if (size > maxBytes)
    {
      int firstNewLine =
          content.indexOf('\n');

      if (firstNewLine >= 0)
      {
        content =
            content.substring(
                firstNewLine + 1);
      }
    }

    return content;
  }

  size_t getFileSize(const char *path)
  {
    if (!hasActiveStorage())
      return 0;

    File file = activeFS->open(
        path,
        FILE_READ);

    if (!file)
      return 0;

    size_t size = file.size();

    file.close();

    return size;
  }

  File openRead(const char *path)
  {
    if (!hasActiveStorage())
      return File();

    return activeFS->open(
        path,
        FILE_READ);
  }

  bool loadTankSettings(float &fullPercent, float &lowPercent, uint8_t &latchMode)
  {
    if (!hasActiveStorage() || !activeFS->exists(tankSettingsFile))
      return false;

    File file = activeFS->open(tankSettingsFile, FILE_READ);
    if (!file)
      return false;

    String line = file.readStringUntil('\n');
    file.close();

    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);
    if (firstComma <= 0 || secondComma <= firstComma)
      return false;

    float savedFull = line.substring(0, firstComma).toFloat();
    float savedLow = line.substring(firstComma + 1, secondComma).toFloat();
    int savedMode = line.substring(secondComma + 1).toInt();

    if (savedFull <= savedLow || savedFull > 100.0f || savedLow < 0.0f || savedMode < 0 || savedMode > 2)
      return false;

    fullPercent = savedFull;
    lowPercent = savedLow;
    latchMode = static_cast<uint8_t>(savedMode);
    return true;
  }

  bool saveTankSettings(float fullPercent, float lowPercent, uint8_t latchMode)
  {
    if (!hasActiveStorage() || fullPercent <= lowPercent || fullPercent > 100.0f || lowPercent < 0.0f || latchMode > 2)
      return false;

    File file = activeFS->open(tankSettingsFile, FILE_WRITE);
    if (!file)
      return false;

    file.print(fullPercent, 1);
    file.print(',');
    file.print(lowPercent, 1);
    file.print(',');
    file.println(latchMode);
    file.close();
    return true;
  }
}
