#include "PreferencesManager.h"
#include "LogManager.h"
#include "config.h"

static const char* TAG = "PreferencesManager";

PreferencesManager preferencesManager;

void PreferencesManager::load() {
  LINK_LOGI(TAG, "Loading preferences...");
  
  preferences.begin(PREF_NAMESPACE, true);
  
  apiKey = preferences.getString(PREF_API_KEY, "");
  hostname = preferences.getString(PREF_HOSTNAME, DEFAULT_HOSTNAME);
  line1Color = preferences.getString(PREF_LINE1_COLOR, DEFAULT_LINE1_COLOR);
  line2Color = preferences.getString(PREF_LINE2_COLOR, DEFAULT_LINE2_COLOR);
  brightness = preferences.getUChar(PREF_BRIGHTNESS, DEFAULT_BRIGHTNESS);
  
  preferences.end();
  
  LINK_LOGI(TAG, "Hostname: %s", hostname.c_str());
  LINK_LOGI(TAG, "Line 1 Color: #%s", line1Color.c_str());
  LINK_LOGI(TAG, "Line 2 Color: #%s", line2Color.c_str());
  LINK_LOGI(TAG, "Brightness: %d", brightness);
}

void PreferencesManager::save() {
  LINK_LOGI(TAG, "Saving preferences...");
  
  preferences.begin(PREF_NAMESPACE, false);
  
  preferences.putString(PREF_API_KEY, apiKey);
  preferences.putString(PREF_HOSTNAME, hostname);
  preferences.putString(PREF_LINE1_COLOR, line1Color);
  preferences.putString(PREF_LINE2_COLOR, line2Color);
  preferences.putUChar(PREF_BRIGHTNESS, brightness);
  
  preferences.end();
  
  LINK_LOGI(TAG, "Preferences saved");
}
