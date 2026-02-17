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
  String getRouteId() const { return routeId; }
  
  void setHomeStation(const String& value) { homeStation = value; }
  void setApiKey(const String& value) { apiKey = value; }
  void setRouteId(const String& value) { routeId = value; }
  
private:
  Preferences preferences;
  String homeStation;
  String apiKey;
  String routeId;
};

extern PreferencesManager preferencesManager;

#endif // PREFERENCESMANAGER_H
