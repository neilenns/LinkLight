#ifndef LEDCONTROLLER_H
#define LEDCONTROLLER_H

#include <NeoPixelBus.h>
#include <map>
#include "config.h"

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
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2811Method> strip{LED_COUNT, LED_PIN};
  
  // Line 1 station to LED mapping
  std::map<String, StationLEDMapping> line1StationMap;
  
  void initializeStationMaps();
  void setAllLEDs(const RgbColor& color);
  void setTrainLED(int ledIndex, const RgbColor& color);
  int getTrainLEDIndex(const TrainData& train);
};

extern LEDController ledController;

#endif // LEDCONTROLLER_H
