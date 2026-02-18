#ifndef COLORS_H
#define COLORS_H

#include <NeoPixelBus.h>
#include <Arduino.h>

// Common LED colors used throughout the application
static const RgbColor COLOR_BLUE = RgbColor(0, 0, 32);
static const RgbColor COLOR_GREEN = RgbColor(0, 32, 0);
static const RgbColor COLOR_YELLOW = RgbColor(32, 32, 0);
static const RgbColor COLOR_BLACK = RgbColor(0, 0, 0);

// Helper class for color management
class ColorManager {
public:
  // Convert hex color string (e.g., "#00ff00") to RgbColor with brightness applied
  static RgbColor hexToRgb(const String& hexColor, uint8_t brightness = 255);
  
  // Get Line 1 color from preferences with brightness applied
  static RgbColor getLine1Color();
  
  // Get Line 2 color from preferences with brightness applied
  static RgbColor getLine2Color();
  
  // Get shared line color (mix of Line 1 and Line 2) with brightness applied
  static RgbColor getSharedLineColor();
};

#endif // COLORS_H
