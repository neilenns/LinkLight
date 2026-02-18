#include "colors.h"
#include "PreferencesManager.h"

RgbColor ColorManager::hexToRgb(const String& hexColor) {
  // Remove '#' if present
  String hex = hexColor;
  if (hex.startsWith("#")) {
    hex = hex.substring(1);
  }
  
  // Default to white if invalid
  if (hex.length() != 6) {
    return RgbColor(255, 255, 255);
  }
  
  // Parse hex values
  long number = strtol(hex.c_str(), NULL, 16);
  uint8_t r = (number >> 16) & 0xFF;
  uint8_t g = (number >> 8) & 0xFF;
  uint8_t b = number & 0xFF;
  
  return RgbColor(r, g, b);
}

RgbColor ColorManager::getLine1Color() {
  String hexColor = preferencesManager.getLine1Color();
  return hexToRgb(hexColor);
}

RgbColor ColorManager::getLine2Color() {
  String hexColor = preferencesManager.getLine2Color();
  return hexToRgb(hexColor);
}

RgbColor ColorManager::getSharedLineColor() {
  String hexColor = preferencesManager.getSharedColor();
  return hexToRgb(hexColor);
}
