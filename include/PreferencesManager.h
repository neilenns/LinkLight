#ifndef PREFERENCESMANAGER_H
#define PREFERENCESMANAGER_H

#include <Preferences.h>
#include <Arduino.h>

class PreferencesManager {
public:
  void load();
  void save();
  
  String getApiKey() const { return apiKey; }
  String getHostname() const { return hostname; }
  String getTimezone() const { return timezone; }
  String getFocusedTripId() const { return focusedTripId; }
  
  void setApiKey(const String& value) { apiKey = value; }
  void setHostname(const String& value) { hostname = value; }
  void setTimezone(const String& value) { timezone = value; }
  void setFocusedTripId(const String& value) { focusedTripId = value; }
  
private:
  Preferences preferences;
  String apiKey;
  String hostname;
  String timezone;
  String focusedTripId;  // Not persisted, runtime only
};

extern PreferencesManager preferencesManager;

#endif // PREFERENCESMANAGER_H
