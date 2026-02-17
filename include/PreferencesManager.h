#ifndef PREFERENCESMANAGER_H
#define PREFERENCESMANAGER_H

#include <Preferences.h>
#include <Arduino.h>

class PreferencesManager {
public:
  void load();
  void save();
  
  String getHomeStation() const { return homeStation; }
  String getApiKey() const { return apiKey; }
  String getHostname() const { return hostname; }
  
  void setHomeStation(const String& value) { homeStation = value; }
  void setApiKey(const String& value) { apiKey = value; }
  void setHostname(const String& value) { hostname = value; }
  
private:
  Preferences preferences;
  String homeStation;
  String apiKey;
  String hostname;
};

extern PreferencesManager preferencesManager;

#endif // PREFERENCESMANAGER_H
