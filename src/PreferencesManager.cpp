#include "PreferencesManager.h"
#include "LogManager.h"
#include "config.h"

static const char* TAG = "PreferencesManager";

PreferencesManager preferencesManager;

void PreferencesManager::load() {
  LINK_LOGI(TAG, "Loading preferences...");
  
  preferences.begin(PREF_NAMESPACE, true);
  
  homeStation = preferences.getString(PREF_HOME_STATION, "");
  apiKey = preferences.getString(PREF_API_KEY, "");
  
  preferences.end();
  
  LINK_LOGI(TAG, "Home Station: %s", homeStation.isEmpty() ? "Not set" : homeStation.c_str());
}

void PreferencesManager::save() {
  LINK_LOGI(TAG, "Saving preferences...");
  
  preferences.begin(PREF_NAMESPACE, false);
  
  preferences.putString(PREF_HOME_STATION, homeStation);
  preferences.putString(PREF_API_KEY, apiKey);
  
  preferences.end();
  
  LINK_LOGI(TAG, "Preferences saved");
}
