#ifndef COLORS_H
#define COLORS_H

#include <NeoPixelBus.h>

// Common LED colors used throughout the application
static const RgbColor COLOR_BLUE = RgbColor(0, 0, 32);
static const RgbColor COLOR_GREEN = RgbColor(0, 32, 0);
static const RgbColor COLOR_YELLOW = RgbColor(32, 32, 0);
static const RgbColor COLOR_BLACK = RgbColor(0, 0, 0);

// Line-specific colors
static const RgbColor LINE_1_COLOR = COLOR_GREEN;
static const RgbColor LINE_2_COLOR = COLOR_BLUE;
static const RgbColor SHARED_LINE_COLOR = COLOR_YELLOW;

#endif // COLORS_H
