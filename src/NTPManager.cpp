#include "NTPManager.h"
#include "LogManager.h"
#include "PreferencesManager.h"
#include <time.h>

static const char* LOG_TAG = "NTPManager";

// NTP servers
const char* NTPManager::NTP_SERVER1 = "pool.ntp.org";
const char* NTPManager::NTP_SERVER2 = "time.nist.gov";
const char* NTPManager::NTP_SERVER3 = "time.google.com";

NTPManager ntpManager;

void NTPManager::setup() {
  LINK_LOGD(LOG_TAG, "Setting up NTP...");
  
  // Get timezone from preferences
  String timezone = preferencesManager.getTimezone();
  
  // Set timezone environment variable
  // Format: TZ string like "PST8PDT,M3.2.0,M11.1.0" for Pacific Time
  setenv("TZ", timezone.c_str(), 1);
  tzset();
  
  // Configure NTP with multiple servers for redundancy
  configTime(0, 0, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
  
  LINK_LOGD(LOG_TAG, "NTP configured with timezone: %s", timezone.c_str());
  
  // Wait a bit for time to synchronize
  int retries = 0;
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo) && retries < 10) {
    delay(500);
    retries++;
  }
  
  if (retries < 10) {
    LINK_LOGI(LOG_TAG, "Current time: %s", asctime(&timeinfo));
  } else {
    LINK_LOGW(LOG_TAG, "Failed to synchronize time");
  }
}
