#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <LittleFS.h>
#include <FS.h>
#include "Pins.h"
#include "sd_card.h"
#include "temp_sensor/temp_sensor.h"
#include "current_sensor/current_sensor.h"
#include "vibration_sensor/vibration_sensor.h"
#include "load_relay/load_relay.h"
#include "maintenance_manager/maintenance_manager.h"

namespace sd_card
{
  static SPIClass sdSPI(FSPI);

  static fs::FS *activeFS = nullptr;

  static bool ready = false;
  static bool sdReady = false;
  static bool internalReady = false;

  static unsigned long lastLog = 0;
  static unsigned long logInterval = 5000;

  static const char *motorLogFile = "/motor_log.csv";
  static const char *analysisLogFile = "/analysis_log.csv";
  static const char *eventLogFile = "/event_log.csv";

  static String backendName = "NONE";

  static String cleanCsvText(String text)
  {
    text.replace("\r", " ");
    text.replace("\n", " ");
    text.replace(",", ";");
    return text;
  }

  static bool hasActiveStorage()
  {
    return ready && activeFS != nullptr;
  }

  static bool fileExists(const char *path)
  {
    if (!hasActiveStorage())
    {
      return false;
    }

    return activeFS->exists(path);
  }

  static bool appendLine(const char *path, const String &line)
  {
    if (!hasActiveStorage())
    {
      return false;
    }

    File file = activeFS->open(path, FILE_APPEND);

    if (!file)
    {
      file = activeFS->open(path, FILE_WRITE);
    }

    if (!file)
    {
      return false;
    }

    file.println(line);
    file.close();

    return true;
  }

  static void createFileWithHeader(const char *path, const String &header)
  {
    if (!hasActiveStorage())
    {
      return;
    }

    if (!fileExists(path))
    {
      File file = activeFS->open(path, FILE_WRITE);

      if (file)
      {
        file.println(header);
        file.close();
      }
    }
  }

  static void createHeaders()
  {
    createFileWithHeader(
        motorLogFile,
        "millis,temp_c,temp_valid,current_a,current_vrms,vibration_rms_g,x_g,y_g,z_g,relay_on,relay_requested,fault,backend");

    createFileWithHeader(
        analysisLogFile,
        "millis,health_score,risk_score,level,worst_metric,forecast_minutes,recommendation");

    createFileWithHeader(
        eventLogFile,
        "millis,event,message,backend");
  }

  static void selectBackend()
  {
    if (sdReady)
    {
      activeFS = &SD;
      backendName = "SD CARD";
      ready = true;
      return;
    }

    if (internalReady)
    {
      activeFS = &LittleFS;
      backendName = "INTERNAL FLASH";
      ready = true;
      return;
    }

    activeFS = nullptr;
    backendName = "NONE";
    ready = false;
  }

  void begin()
  {
    ready = false;
    sdReady = false;
    internalReady = false;
    activeFS = nullptr;
    backendName = "NONE";

    internalReady = LittleFS.begin(true);

    sdSPI.begin(Pins::SD_SCK, Pins::SD_MISO, Pins::SD_MOSI, Pins::SD_CS);
    sdReady = SD.begin(Pins::SD_CS, sdSPI);

    selectBackend();

    if (ready)
    {
      createHeaders();

      String message = "Storage backend selected: " + backendName;
      logEvent("BOOT", message);
    }
  }

  void update()
  {
    if (!ready)
    {
      return;
    }

    if (millis() - lastLog >= logInterval)
    {
      lastLog = millis();
      logNow();
    }
  }

  void logNow()
  {
    if (!ready)
    {
      return;
    }

    maintenance_manager::Snapshot snap = maintenance_manager::getSnapshot();

    String motorLine;

    motorLine += String(millis());
    motorLine += ",";

    if (temp_sensor::isValid())
    {
      motorLine += String(temp_sensor::getTemperatureC(), 2);
    }
    else
    {
      motorLine += "nan";
    }

    motorLine += ",";
    motorLine += temp_sensor::isValid() ? "1" : "0";
    motorLine += ",";
    motorLine += String(current_sensor::getCurrentA(), 3);
    motorLine += ",";
    motorLine += String(current_sensor::getVoltageRMS(), 4);
    motorLine += ",";
    motorLine += String(vibration_sensor::getVibrationRMS(), 3);
    motorLine += ",";
    motorLine += String(vibration_sensor::getX(), 3);
    motorLine += ",";
    motorLine += String(vibration_sensor::getY(), 3);
    motorLine += ",";
    motorLine += String(vibration_sensor::getZ(), 3);
    motorLine += ",";
    motorLine += load_relay::isOn() ? "1" : "0";
    motorLine += ",";
    motorLine += load_relay::getRequestedState() ? "1" : "0";
    motorLine += ",";
    motorLine += load_relay::isFault() ? "1" : "0";
    motorLine += ",";
    motorLine += backendName;

    appendLine(motorLogFile, motorLine);

    String analysisLine;

    analysisLine += String(millis());
    analysisLine += ",";
    analysisLine += String(snap.healthScore, 1);
    analysisLine += ",";
    analysisLine += String(snap.riskScore, 1);
    analysisLine += ",";
    analysisLine += maintenance_manager::getLevelText();
    analysisLine += ",";
    analysisLine += cleanCsvText(maintenance_manager::getWorstMetric());
    analysisLine += ",";
    analysisLine += String(snap.forecastMinutes, 1);
    analysisLine += ",";
    analysisLine += cleanCsvText(maintenance_manager::getRecommendation());

    appendLine(analysisLogFile, analysisLine);
  }

  void logEvent(const String &event, const String &message)
  {
    if (!ready)
    {
      return;
    }

    String line;

    line += String(millis());
    line += ",";
    line += cleanCsvText(event);
    line += ",";
    line += cleanCsvText(message);
    line += ",";
    line += backendName;

    appendLine(eventLogFile, line);
  }

  bool isReady()
  {
    return ready;
  }

  bool isSdReady()
  {
    return sdReady;
  }

  bool isInternalReady()
  {
    return internalReady;
  }

  String getBackendName()
  {
    return backendName;
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
    {
      return "";
    }

    File file = activeFS->open(path, FILE_READ);

    if (!file)
    {
      return "";
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

    return content;
  }

  String readTail(const char *path, size_t maxBytes)
  {
    if (!hasActiveStorage())
    {
      return "";
    }

    File file = activeFS->open(path, FILE_READ);

    if (!file)
    {
      return "";
    }

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

    int firstNewLine = content.indexOf('\n');

    if (size > maxBytes && firstNewLine >= 0)
    {
      content = content.substring(firstNewLine + 1);
    }

    return content;
  }

  size_t getFileSize(const char *path)
  {
    if (!hasActiveStorage())
    {
      return 0;
    }

    File file = activeFS->open(path, FILE_READ);

    if (!file)
    {
      return 0;
    }

    size_t size = file.size();
    file.close();

    return size;
  }

  File openRead(const char *path)
  {
    if (!hasActiveStorage())
    {
      return File();
    }

    return activeFS->open(path, FILE_READ);
  }
}
