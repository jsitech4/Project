#include <Arduino.h>
#include <LittleFS.h>
#include <FS.h>
#include "storage.h"
#include "temp_sensor/temp_sensor.h"
#include "vibration_sensor/vibration_sensor.h"
#include "load_relay/load_relay.h"
#include "maintenance_manager/maintenance_manager.h"

namespace storage
{
  static fs::FS *activeFS = nullptr;
  static bool ready = false;
  static bool internalReady = false;

  static unsigned long lastLog = 0;
  static unsigned long logInterval = 5000;

  static const char *motorLogFile = "/motor_log.csv";
  static const char *analysisLogFile = "/analysis_log.csv";
  static const char *eventLogFile = "/event_log.csv";

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
        motorLogFile,
        "millis,"
        "temp_c,"
        "temp_valid,"
        "vibration1_rms_g,"
        "vibration2_rms_g,"
        "vibration1_x_g,"
        "vibration1_y_g,"
        "vibration1_z_g,"
        "vibration2_x_g,"
        "vibration2_y_g,"
        "vibration2_z_g,"
        "relay_on,"
        "relay_requested,"
        "fault");

    createFileWithHeader(
        analysisLogFile,
        "millis,"
        "health_score,"
        "risk_score,"
        "level,"
        "worst_metric,"
        "forecast_minutes,"
        "recommendation");

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

    String motorLine;

    motorLine += String(millis());
    motorLine += ",";

    // Temperature
    if (temp_sensor::isValid())
    {
      motorLine += String(
          temp_sensor::getTemperatureC(),
          2);
    }
    else
    {
      motorLine += "nan";
    }

    motorLine += ",";

    // Temperature validity
    motorLine +=
        temp_sensor::isValid() ? "1" : "0";

    motorLine += ",";

    motorLine += String(
        vibration_sensor::getSensor1VibrationRMS(),
        3);

    motorLine += ",";

    motorLine += String(
        vibration_sensor::getSensor2VibrationRMS(),
        3);

    motorLine += ",";

    motorLine += String(
        vibration_sensor::getSensor1X(),
        4);

    motorLine += ",";

    motorLine += String(
        vibration_sensor::getSensor1Y(),
        4);

    motorLine += ",";

    motorLine += String(
        vibration_sensor::getSensor1Z(),
        4);

    motorLine += ",";

    motorLine += String(
        vibration_sensor::getSensor2X(),
        4);

    motorLine += ",";

    motorLine += String(
        vibration_sensor::getSensor2Y(),
        4);

    motorLine += ",";

    motorLine += String(
        vibration_sensor::getSensor2Z(),
        4);

    motorLine += ",";

    motorLine +=
        load_relay::isOn() ? "1" : "0";

    motorLine += ",";

    motorLine +=
        load_relay::getRequestedState() ? "1" : "0";

    motorLine += ",";

    motorLine +=
        load_relay::isFault() ? "1" : "0";

    appendLine(
        motorLogFile,
        motorLine);

    String analysisLine;

    analysisLine += String(millis());
    analysisLine += ",";

    analysisLine += String(
        snap.healthScore,
        1);

    analysisLine += ",";

    analysisLine += String(
        snap.riskScore,
        1);

    analysisLine += ",";

    analysisLine +=
        maintenance_manager::getLevelText();

    analysisLine += ",";

    analysisLine += cleanCsvText(
        maintenance_manager::getWorstMetric());

    analysisLine += ",";

    analysisLine += String(
        snap.forecastMinutes,
        1);

    analysisLine += ",";

    analysisLine += cleanCsvText(
        maintenance_manager::getRecommendation());

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
    return motorLogFile;
  }

  const char *getMotorLogFileName()
  {
    return motorLogFile;
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
}
