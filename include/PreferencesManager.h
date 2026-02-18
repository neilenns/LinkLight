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
  
  void setApiKey(const String& value) { apiKey = value; }
  void setHostname(const String& value) { hostname = value; }
  void setTimezone(const String& value) { timezone = value; }
  
private:
  Preferences preferences;
  String apiKey;
  String hostname;
  String timezone;
};

extern PreferencesManager preferencesManager;

#endif // PREFERENCESMANAGER_H
