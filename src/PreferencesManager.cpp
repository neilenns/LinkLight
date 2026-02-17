#include "PreferencesManager.h"
#include <esp_log.h>
#include "config.h"

static const char* TAG = "PreferencesManager";

PreferencesManager preferencesManager;

void PreferencesManager::load() {
  ESP_LOGI(TAG, "Loading preferences...");
  
  preferences.begin(PREF_NAMESPACE, true);
  
  homeStation = preferences.getString(PREF_HOME_STATION, "");
  apiKey = preferences.getString(PREF_API_KEY, "");
  routeId = preferences.getString(PREF_ROUTE_ID, DEFAULT_ROUTE_ID);
  
  preferences.end();
  
  ESP_LOGI(TAG, "Home Station: %s", homeStation.isEmpty() ? "Not set" : homeStation.c_str());
  ESP_LOGI(TAG, "Route ID: %s", routeId.c_str());
}

void PreferencesManager::save() {
  ESP_LOGI(TAG, "Saving preferences...");
  
  preferences.begin(PREF_NAMESPACE, false);
  
  preferences.putString(PREF_HOME_STATION, homeStation);
  preferences.putString(PREF_API_KEY, apiKey);
  preferences.putString(PREF_ROUTE_ID, routeId);
  
  preferences.end();
  
  ESP_LOGI(TAG, "Preferences saved");
}
