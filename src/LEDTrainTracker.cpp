#include "LEDTrainTracker.h"
#include "LogManager.h"
#include "config.h"
#include "TrainDataManager.h"
#include "colors.h"

static const char* LOG_TAG = "LEDTrainTracker";

LEDTrainTracker::LEDTrainTracker() {
  reset();
}

// Add a train at a specific LED. Used later when displaying LEDs to determine color based on train presence.
void LEDTrainTracker::addTrain(int ledIndex, Line line, const String& vehicleId) {
  if (ledIndex < 0 || ledIndex >= LED_COUNT) {
    LINK_LOGW(LOG_TAG, "Invalid LED index %d for line %d", ledIndex, static_cast<int>(line));
    return;
  }
  ledTrains[ledIndex].push_back({vehicleId, line});
}

// Reset all trains for all LEDs. Called at the start of each update cycle to clear previous state.
void LEDTrainTracker::reset() {
  for (int i = 0; i < LED_COUNT; i++) {
    ledTrains[i].clear();
  }
}

// Display the trains on the LED strip based on current state. Determines color for each LED based on presence of trains from both lines.
void LEDTrainTracker::display(NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Apa106Method>& strip) {
  for (int i = 0; i < LED_COUNT; i++) {
    bool hasLine1 = false;
    bool hasLine2 = false;
    for (const TrainAtLED& t : ledTrains[i]) {
      if (t.line == Line::LINE_1) hasLine1 = true;
      if (t.line == Line::LINE_2) hasLine2 = true;
    }

    if (hasLine1 && hasLine2) {
      strip.SetPixelColor(i, ColorManager::getSharedLineColor());
    } else if (hasLine1) {
      strip.SetPixelColor(i, ColorManager::getLine1Color());
    } else if (hasLine2) {
      strip.SetPixelColor(i, ColorManager::getLine2Color());
    } else {
      strip.SetPixelColor(i, COLOR_BLACK);
    }
  }

  strip.Show();
}

const std::vector<TrainAtLED>& LEDTrainTracker::getTrainsAtLED(int ledIndex) const {
  return ledTrains[ledIndex];
}
