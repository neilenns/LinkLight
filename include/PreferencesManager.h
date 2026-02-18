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
  String getFocusedVehicleId() const { return focusedVehicleId; }
  
  void setApiKey(const String& value) { apiKey = value; }
  void setHostname(const String& value) { hostname = value; }
  void setTimezone(const String& value) { timezone = value; }
  void setFocusedVehicleId(const String& value) { focusedVehicleId = value; }
  
private:
  Preferences preferences;
  String apiKey;
  String hostname;
  String timezone;
  String focusedVehicleId;  // Not persisted, runtime only
};

extern PreferencesManager preferencesManager;

#endif // PREFERENCESMANAGER_H
