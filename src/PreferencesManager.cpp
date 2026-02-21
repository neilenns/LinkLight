#include "PreferencesManager.h"
#include "LogManager.h"
#include "config.h"

static const char* LOG_TAG = "PreferencesManager";

PreferencesManager preferencesManager;

void PreferencesManager::load() {
  LINK_LOGD(LOG_TAG, "Loading preferences...");
  
  preferences.begin(PREF_NAMESPACE, true);
  
  apiKey = preferences.getString(PREF_API_KEY, "");
  hostname = preferences.getString(PREF_HOSTNAME, DEFAULT_HOSTNAME);
  timezone = preferences.getString(PREF_TIMEZONE, DEFAULT_TIMEZONE);
  updateInterval = preferences.getUInt(PREF_UPDATE_INTERVAL, DEFAULT_UPDATE_INTERVAL);
  atStationThreshold = preferences.getUInt(PREF_AT_STATION_THRESHOLD, DEFAULT_AT_STATION_THRESHOLD);
  line1Color = preferences.getString(PREF_LINE1_COLOR, DEFAULT_LINE1_COLOR);
  line2Color = preferences.getString(PREF_LINE2_COLOR, DEFAULT_LINE2_COLOR);
  sharedColor = preferences.getString(PREF_SHARED_COLOR, DEFAULT_SHARED_COLOR);
  
  preferences.end();
  
  // focusedVehicleId is not persisted - always starts empty on boot
  focusedVehicleId = "";
  }

void PreferencesManager::save() {
  LINK_LOGD(LOG_TAG, "Saving preferences...");
  
  preferences.begin(PREF_NAMESPACE, false);
  
  preferences.putString(PREF_API_KEY, apiKey);
  preferences.putString(PREF_HOSTNAME, hostname);
  preferences.putString(PREF_TIMEZONE, timezone);
  preferences.putUInt(PREF_UPDATE_INTERVAL, updateInterval);
  preferences.putUInt(PREF_AT_STATION_THRESHOLD, atStationThreshold);
  preferences.putString(PREF_LINE1_COLOR, line1Color);
  preferences.putString(PREF_LINE2_COLOR, line2Color);
  preferences.putString(PREF_SHARED_COLOR, sharedColor);
  
  preferences.end();
  
  LINK_LOGD(LOG_TAG, "Preferences saved");
}
