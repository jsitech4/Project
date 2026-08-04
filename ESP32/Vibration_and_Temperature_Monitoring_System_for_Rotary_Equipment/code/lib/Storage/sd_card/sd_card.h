#ifndef SD_CARD_H
#define SD_CARD_H

#include <Arduino.h>
#include <FS.h>

namespace sd_card
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
  const char *getMotorLogFileName();
  const char *getAnalysisLogFileName();
  const char *getEventLogFileName();

  String readFile(const char *path);
  String readTail(const char *path, size_t maxBytes);

  size_t getFileSize(const char *path);

  File openRead(const char *path);
}

#endif
