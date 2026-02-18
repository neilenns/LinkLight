#ifndef LEDTRAINTRACKER_H
#define LEDTRAINTRACKER_H

#include <NeoPixelBus.h>
#include "config.h"
#include "colors.h"

// Structure to track train counts for both lines at a single LED
struct LEDTrainCounts {
  int line1Count = 0;
  int line2Count = 0;
};

// Class to track train counts at each LED position
class LEDTrainTracker {
public:
  LEDTrainTracker();
  
  // Increment the count of trains for a specific line at a specific LED
  void incrementTrainCount(int ledIndex, const String& line);
  
  // Reset all train counts to zero
  void reset();
  
  // Display the trains on the LED strip based on current counts
  // Yellow for both lines, line color for single line, black for no trains
  void display(NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2811Method>& strip);
  
  // Log the train counts for northbound (0-53) and southbound (108-56) stations
  void logTrainCounts();
  
private:
  // Array to track train counts for each LED using a struct
  // Index represents LED position (0-113)
  LEDTrainCounts ledCounts[LED_COUNT];
};

#endif // LEDTRAINTRACKER_H
