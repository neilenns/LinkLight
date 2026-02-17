#ifndef LEDCONTROLLER_H
#define LEDCONTROLLER_H

#include <NeoPixelBus.h>
#include <map>
#include "config.h"

// Forward declaration
struct TrainData;

// Station LED mapping structure: (southbound_index, leds_from_prev), (northbound_index, leds_from_prev)
struct StationLEDMapping {
  int southboundIndex;
  int southboundLedsFromPrev;
  int northboundIndex;
  int northboundLedsFromPrev;
};

class LEDController {
public:
  void setup();
  void displayTrainPositions();
  
private:
  NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> strip{LED_COUNT, LED_PIN};
  
  // Line 1 station to LED mapping
  std::map<String, StationLEDMapping> line1StationMap;
  
  void initializeStationMaps();
  void clearAllLEDs();
  void setTrainLED(int ledIndex, const RgbColor& color);
  int getTrainLEDIndex(const TrainData& train);
};

extern LEDController ledController;

#endif // LEDCONTROLLER_H
