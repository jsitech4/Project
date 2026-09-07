#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <FS.h>

namespace storage
{
  void begin();
  void update();

  void logNow();
  void logEvent(const String &event, const String &message);

  bool isReady();
  bool isSdReady();
  bool isInternalReady();

  String getBackendName();

  void setLogInterval(unsigned long intervalMs);

  const char *getFileName();
  const char *getLiquidLogFileName();
  const char *getAnalysisLogFileName();
  const char *getEventLogFileName();

  String readFile(const char *path);
  String readTail(const char *path, size_t maxBytes);

  size_t getFileSize(const char *path);

  File openRead(const char *path);

  bool loadTankSettings(float &fullPercent, float &lowPercent, uint8_t &latchMode);
  bool saveTankSettings(float fullPercent, float lowPercent, uint8_t latchMode);
}

#endif
