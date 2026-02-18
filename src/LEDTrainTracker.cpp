#include "LEDTrainTracker.h"
#include "LogManager.h"
#include "config.h"

static const char* LOG_TAG = "LEDTrainTracker";

LEDTrainTracker::LEDTrainTracker() {
  reset();
}

// Increment the count of trains for a specific line at a specific LED.
// Used later when displaying LEDs to determine color based on train presence.
void LEDTrainTracker::incrementTrainCount(int ledIndex, const String& line) {
  // Validate LED index
  if (ledIndex < 0 || ledIndex >= LED_COUNT) {
    LINK_LOGW(LOG_TAG, "Invalid LED index %d for line %s", ledIndex, line.c_str());
    return;
  }
  
  // Increment the appropriate counter based on the line
  if (line == LINE_1_NAME) {
    ledCounts[ledIndex].line1Count++;
  } else if (line == LINE_2_NAME) {
    ledCounts[ledIndex].line2Count++;
  } else {
    LINK_LOGW(LOG_TAG, "Unknown line: %s", line.c_str());
  }
}

// Reset all train counts to zero for all LEDs. Called at the start of each update cycle to clear previous counts.
void LEDTrainTracker::reset() {
  for (int i = 0; i < LED_COUNT; i++) {
    ledCounts[i].line1Count = 0;
    ledCounts[i].line2Count = 0;
  }
}

// Display the trains on the LED strip based on current counts. Determines color for each LED based on presence of trains from both lines.
void LEDTrainTracker::display(NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2811Method>& strip) {
  // Display trains on LEDs based on counts
  for (int i = 0; i < LED_COUNT; i++) {
    int totalTrains = ledCounts[i].line1Count + ledCounts[i].line2Count;
    
    if (totalTrains == 0) {
      // No trains at this LED - turn it black
      strip.SetPixelColor(i, COLOR_BLACK);
    } else if (ledCounts[i].line1Count > 0 && ledCounts[i].line2Count > 0) {
      // Both lines have trains at this LED - display yellow
      strip.SetPixelColor(i, SHARED_LINE_COLOR);
    } else if (ledCounts[i].line1Count > 0) {
      // Only Line 1 has trains - display Line 1 color
      strip.SetPixelColor(i, LINE_1_COLOR);
    } else {
      // Only Line 2 has trains - display Line 2 color
      strip.SetPixelColor(i, LINE_2_COLOR);
    }
  }
  
  // Update the LED strip with all changes
  strip.Show();
}

void LEDTrainTracker::logTrainCounts() {
  // Log northbound LEDs (0 up to 54)
  String northboundLog = "Northbound:  ";
  for (int i = 0; i <= 54; i++) {
    int totalTrains = ledCounts[i].line1Count + ledCounts[i].line2Count;
    northboundLog += String(totalTrains);
    if (i < 54) {
      northboundLog += " ";
    }
  }
  LINK_LOGI(LOG_TAG, "%s", northboundLog.c_str());
  
  // Log southbound LEDs (109 down to 55)
  String southboundLog = "Southbound: ";
  for (int i = 109; i >= 55; i--) {
    int totalTrains = ledCounts[i].line1Count + ledCounts[i].line2Count;
    southboundLog += String(totalTrains);
    if (i > 55) {
      southboundLog += " ";
    }
  }
  LINK_LOGI(LOG_TAG, "%s", southboundLog.c_str());
}
