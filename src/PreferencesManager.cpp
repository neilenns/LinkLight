#include "PreferencesManager.h"
#include "LogManager.h"
#include "config.h"

static const char* LOG_TAG = "PreferencesManager";

PreferencesManager preferencesManager;

void PreferencesManager::load() {
  LINK_LOGI(LOG_TAG, "Loading preferences...");
  
  preferences.begin(PREF_NAMESPACE, true);
  
  apiKey = preferences.getString(PREF_API_KEY, "");
  hostname = preferences.getString(PREF_HOSTNAME, DEFAULT_HOSTNAME);
  timezone = preferences.getString(PREF_TIMEZONE, DEFAULT_TIMEZONE);
  
  preferences.end();
  
  LINK_LOGI(LOG_TAG, "Hostname: %s", hostname.c_str());
}

void PreferencesManager::save() {
  LINK_LOGI(LOG_TAG, "Saving preferences...");
  
  preferences.begin(PREF_NAMESPACE, false);
  
  preferences.putString(PREF_API_KEY, apiKey);
  preferences.putString(PREF_HOSTNAME, hostname);
  preferences.putString(PREF_TIMEZONE, timezone);
  
  preferences.end();
  
  LINK_LOGI(LOG_TAG, "Preferences saved");
}
