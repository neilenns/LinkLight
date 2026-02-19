#ifndef LEDTRAINTRACKER_H
#define LEDTRAINTRACKER_H

#include <NeoPixelBus.h>
#include <vector>
#include "config.h"
#include "colors.h"

// Forward declaration of Line enum from TrainDataManager.h
enum class Line;

// Structure to represent a single train at an LED position
struct TrainAtLED {
  String vehicleId;
  Line line;
};

// Class to track trains at each LED position
class LEDTrainTracker {
public:
  LEDTrainTracker();
  
  // Add a train at a specific LED
  void addTrain(int ledIndex, Line line, const String& vehicleId);
  
  // Reset all trains for all LEDs
  void reset();
  
  // Display the trains on the LED strip based on current state
  // Yellow for both lines, line color for single line, black for no trains
  void display(NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Apa106Method>& strip);

  // Get read-only access to the trains at a specific LED
  const std::vector<TrainAtLED>& getTrainsAtLED(int ledIndex) const;
  
private:
  // Array of train lists, one per LED
  std::vector<TrainAtLED> ledTrains[LED_COUNT];
};

#endif // LEDTRAINTRACKER_H
