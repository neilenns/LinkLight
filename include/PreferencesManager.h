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
  
  void setHomeStation(const String& value) { homeStation = value; }
  void setApiKey(const String& value) { apiKey = value; }
  
private:
  Preferences preferences;
  String homeStation;
  String apiKey;
};

extern PreferencesManager preferencesManager;

#endif // PREFERENCESMANAGER_H
