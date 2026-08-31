#ifndef LOCAL_SERVER_H
#define LOCAL_SERVER_H

#include <Arduino.h>

namespace local_server
{
  void begin();
  void update();

  bool isRunning();
  String getIp();
  String getSsid();
}

#endif
