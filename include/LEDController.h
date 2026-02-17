#ifndef LEDCONTROLLER_H
#define LEDCONTROLLER_H

#include <NeoPixelBus.h>
#include "config.h"

class LEDController {
public:
  void setup();
  void displayTrainPositions();
  NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod>& getStrip() { return strip; }
  
private:
  NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> strip{LED_COUNT, LED_PIN};
};

extern LEDController ledController;

#endif // LEDCONTROLLER_H
