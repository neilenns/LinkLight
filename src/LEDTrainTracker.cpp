#include "LEDTrainTracker.h"
#include "LogManager.h"
#include "config.h"
#include "TrainDataManager.h"

static const char* LOG_TAG = "LEDTrainTracker";

LEDTrainTracker::LEDTrainTracker() {
  reset();
}

// Increment the count of trains for a specific line at a specific LED.
// Used later when displaying LEDs to determine color based on train presence.
void LEDTrainTracker::incrementTrainCount(int ledIndex, Line line) {
  // Validate LED index
  if (ledIndex < 0 || ledIndex >= LED_COUNT) {
    LINK_LOGW(LOG_TAG, "Invalid LED index %d for line %d", ledIndex, static_cast<int>(line));
    return;
  }
  
  // Increment the appropriate counter based on the line
  if (line == Line::LINE_1) {
    ledCounts[ledIndex].line1Count++;
  } else if (line == Line::LINE_2) {
    ledCounts[ledIndex].line2Count++;
  } else {
    LINK_LOGW(LOG_TAG, "Unknown line: %d", static_cast<int>(line));
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
void LEDTrainTracker::display(NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Apa106Method>& strip) {
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

// Logs the train counts in two lines, one for northbound and one for southbound LEDs.
// Southbound: 0 0 0 0 0 0 0 0 0 2 0 1 0 0 0 0 0 0 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
// Northbound: 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
void LEDTrainTracker::logTrainCounts() {
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
}
