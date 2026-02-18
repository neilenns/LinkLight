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
  unsigned int getUpdateInterval() const { return updateInterval; }
  String getLine1Color() const { return line1Color; }
  String getLine2Color() const { return line2Color; }
  String getSharedColor() const { return sharedColor; }
  
  void setApiKey(const String& value) { apiKey = value; }
  void setHostname(const String& value) { hostname = value; }
  void setTimezone(const String& value) { timezone = value; }
  void setFocusedVehicleId(const String& value) { focusedVehicleId = value; }
  void setUpdateInterval(unsigned int value) { updateInterval = value; }
  void setLine1Color(const String& value) { line1Color = value; }
  void setLine2Color(const String& value) { line2Color = value; }
  void setSharedColor(const String& value) { sharedColor = value; }
  
private:
  Preferences preferences;
  String apiKey;
  String hostname;
  String timezone;
  String focusedVehicleId;  // Not persisted, runtime only
  unsigned int updateInterval;  // Update interval in seconds
  String line1Color;  // Hex color for Line 1 (e.g., "#00ff00")
  String line2Color;  // Hex color for Line 2 (e.g., "#0000ff")
  String sharedColor;  // Hex color for shared/overlap (e.g., "#ffff00")
};

extern PreferencesManager preferencesManager;

#endif // PREFERENCESMANAGER_H
