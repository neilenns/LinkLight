#include "colors.h"
#include "PreferencesManager.h"

RgbColor ColorManager::hexToRgb(const String& hexColor, uint8_t brightness) {
  // Remove '#' if present
  String hex = hexColor;
  if (hex.startsWith("#")) {
    hex = hex.substring(1);
  }
  
  // Default to white if invalid
  if (hex.length() != 6) {
    return RgbColor(brightness, brightness, brightness);
  }
  
  // Parse hex values
  long number = strtol(hex.c_str(), NULL, 16);
  uint8_t r = (number >> 16) & 0xFF;
  uint8_t g = (number >> 8) & 0xFF;
  uint8_t b = number & 0xFF;
  
  // Apply brightness scaling
  r = (r * brightness) / 255;
  g = (g * brightness) / 255;
  b = (b * brightness) / 255;
  
  return RgbColor(r, g, b);
}

RgbColor ColorManager::getLine1Color() {
  String hexColor = preferencesManager.getLine1Color();
  uint8_t brightness = preferencesManager.getBrightness();
  return hexToRgb(hexColor, brightness);
}

RgbColor ColorManager::getLine2Color() {
  String hexColor = preferencesManager.getLine2Color();
  uint8_t brightness = preferencesManager.getBrightness();
  return hexToRgb(hexColor, brightness);
}

RgbColor ColorManager::getSharedLineColor() {
  // Mix Line 1 and Line 2 colors
  RgbColor line1 = getLine1Color();
  RgbColor line2 = getLine2Color();
  
  uint8_t r = (line1.R + line2.R) / 2;
  uint8_t g = (line1.G + line2.G) / 2;
  uint8_t b = (line1.B + line2.B) / 2;
  
  return RgbColor(r, g, b);
}
