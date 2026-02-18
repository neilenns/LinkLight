#include "LEDTrainTracker.h"
#include "LogManager.h"
#include "config.h"

static const char* LOG_TAG = "LEDTrainTracker";

// Define LED colors
const RgbColor LEDTrainTracker::COLOR_BLUE = RgbColor(0, 0, 32);
const RgbColor LEDTrainTracker::COLOR_GREEN = RgbColor(0, 32, 0);
const RgbColor LEDTrainTracker::COLOR_YELLOW = RgbColor(32, 32, 0);
const RgbColor LEDTrainTracker::COLOR_BLACK = RgbColor(0, 0, 0);

LEDTrainTracker::LEDTrainTracker() {
  reset();
}

void LEDTrainTracker::incrementTrainCount(int ledIndex, const String& line) {
  // Validate LED index
  if (ledIndex < 0 || ledIndex >= LED_COUNT) {
    LINK_LOGW(LOG_TAG, "Invalid LED index %d for line %s", ledIndex, line.c_str());
    return;
  }
  
  // Increment the appropriate counter based on the line
  if (line == LINE_1_NAME) {
    line1Counts[ledIndex]++;
  } else if (line == LINE_2_NAME) {
    line2Counts[ledIndex]++;
  } else {
    LINK_LOGW(LOG_TAG, "Unknown line: %s", line.c_str());
  }
}

void LEDTrainTracker::reset() {
  // Reset all counts to zero
  for (int i = 0; i < LED_COUNT; i++) {
    line1Counts[i] = 0;
    line2Counts[i] = 0;
  }
}

void LEDTrainTracker::display(NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2811Method>& strip) {
  // Display trains on LEDs based on counts
  for (int i = 0; i < LED_COUNT; i++) {
    int totalTrains = line1Counts[i] + line2Counts[i];
    
    if (totalTrains == 0) {
      // No trains at this LED - turn it black
      strip.SetPixelColor(i, COLOR_BLACK);
    } else if (line1Counts[i] > 0 && line2Counts[i] > 0) {
      // Both lines have trains at this LED - display yellow
      strip.SetPixelColor(i, COLOR_YELLOW);
    } else if (line1Counts[i] > 0) {
      // Only Line 1 has trains - display Line 1 color (green)
      strip.SetPixelColor(i, COLOR_GREEN);
    } else {
      // Only Line 2 has trains - display Line 2 color (blue)
      strip.SetPixelColor(i, COLOR_BLUE);
    }
  }
  
  // Update the LED strip with all changes
  strip.Show();
}

void LEDTrainTracker::logTrainCounts() {
  // Log northbound LEDs (0-53)
  String northboundLog = "Northbound (0-53): ";
  for (int i = 0; i <= 53; i++) {
    int totalTrains = line1Counts[i] + line2Counts[i];
    northboundLog += String(totalTrains);
    if (i < 53) {
      northboundLog += " ";
    }
  }
  LINK_LOGI(LOG_TAG, "%s", northboundLog.c_str());
  
  // Log southbound LEDs (108 down to 56)
  String southboundLog = "Southbound (108-56): ";
  for (int i = 108; i >= 56; i--) {
    int totalTrains = line1Counts[i] + line2Counts[i];
    southboundLog += String(totalTrains);
    if (i > 56) {
      southboundLog += " ";
    }
  }
  LINK_LOGI(LOG_TAG, "%s", southboundLog.c_str());
}
