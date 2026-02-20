#ifndef COLORS_H
#define COLORS_H

#include <NeoPixelBus.h>
#include <Arduino.h>

// Common LED colors used throughout the application
static const RgbColor COLOR_BLUE = RgbColor(0x00, 0x7c, 0xAD); // Official SoundTransit blue
static const RgbColor COLOR_GREEN = RgbColor(0x28, 0x81, 0x3F); // Official SoundTransit green
static const RgbColor COLOR_YELLOW = RgbColor(32, 32, 0);
static const RgbColor COLOR_BLACK = RgbColor(0, 0, 0);

// Helper class for color management
class ColorManager {
public:
  // Convert hex color string (e.g., "#00ff00") to RgbColor
  static RgbColor hexToRgb(const String& hexColor);
  
  // Get Line 1 color from preferences
  static RgbColor getLine1Color();
  
  // Get Line 2 color from preferences
  static RgbColor getLine2Color();
  
  // Get shared line color from preferences (when both lines have trains at same LED)
  static RgbColor getSharedLineColor();
};

#endif // COLORS_H
