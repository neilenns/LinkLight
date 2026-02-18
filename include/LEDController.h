#ifndef LEDCONTROLLER_H
#define LEDCONTROLLER_H

#include <NeoPixelBus.h>
#include <map>
#include "config.h"
#include "LEDTrainTracker.h"

// Forward declaration
struct TrainData;

// Station LED mapping structure
struct StationLEDMapping {
  int northboundIndex;
  int southboundIndex;
};

class LEDController {
public:
  void setup();
  void startupAnimation();
  void displayTrainPositions();
  void testStationLEDs(const String& stationName);
  const std::map<String, StationLEDMapping>& getStationMap() const;

private:
// Setup for WS2815 LEDs. Yes, it's using Apa106 method, but according to https://github.com/Makuna/NeoPixelBus/pull/795#issuecomment-2466545330
// that's the one that's closest in timing and works. I've also tried the NeoEsp32Rmt0Ws2811Method and while it worked I was seeing
// the occasional LED displayed in the wrong position.
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Apa106Method> strip{LED_COUNT, LED_PIN};
  
  // Station to LED mappings
  std::map<String, StationLEDMapping> stationMap;
  
  // Train tracker for handling multiple trains at same LED
  LEDTrainTracker trainTracker;
  
  void initializeStationMaps();
  void setAllLEDs(const RgbColor& color);
  int getTrainLEDIndex(const TrainData& train);
};

extern LEDController ledController;

#endif // LEDCONTROLLER_H
