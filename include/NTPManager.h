#ifndef NTPMANAGER_H
#define NTPMANAGER_H

#include <Arduino.h>

class NTPManager {
public:
  void setup();
  
private:
  static const char* NTP_SERVER1;
  static const char* NTP_SERVER2;
  static const char* NTP_SERVER3;
};

extern NTPManager ntpManager;

#endif // NTPMANAGER_H
