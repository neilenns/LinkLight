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
  String getLine1Color() const { return line1Color; }
  String getLine2Color() const { return line2Color; }
  uint8_t getBrightness() const { return brightness; }
  
  void setApiKey(const String& value) { apiKey = value; }
  void setHostname(const String& value) { hostname = value; }
  void setLine1Color(const String& value) { line1Color = value; }
  void setLine2Color(const String& value) { line2Color = value; }
  void setBrightness(uint8_t value) { brightness = value; }
  
private:
  Preferences preferences;
  String apiKey;
  String hostname;
  String line1Color;
  String line2Color;
  uint8_t brightness;
};

extern PreferencesManager preferencesManager;

#endif // PREFERENCESMANAGER_H
