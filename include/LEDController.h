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

private:
// Setup for WS2812x LEDs.
//  NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> strip{LED_COUNT, LED_PIN};

// Setup for WS2815 LEDs. Yes, it's using 2811 method, but according to https://github.com/Makuna/NeoPixelBus/pull/795#issuecomment-2061333595
// that's the one that's closest in timing and works.
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
