#include "LEDTrainTracker.h"
#include "LogManager.h"
#include "config.h"
#include "TrainDataManager.h"
#include "colors.h"

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
      // Both lines have trains at this LED - display mixed color
      strip.SetPixelColor(i, ColorManager::getSharedLineColor());
    } else if (ledCounts[i].line1Count > 0) {
      // Only Line 1 has trains - display Line 1 color
      strip.SetPixelColor(i, ColorManager::getLine1Color());
    } else {
      // Only Line 2 has trains - display Line 2 color
      strip.SetPixelColor(i, ColorManager::getLine2Color());
    }
  }
  
  // Update the LED strip with all changes
  strip.Show();
}

// Logs the train counts in four lines, one for northbound and one for southbound LEDs for each train line.
// The extra spaces in the Line 2 logs are intentional and align the digits with how things look on the physical LED layout,
// with the between-station LEDs between Judkins Park and ID/Chinatown aligning above ID/Chinatown.
// 00:01:30[I] LEDTrainTracker:Line 2 southbound:       0 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 1 0 0 0 1 0 0 0 0
// 00:01:30[I] LEDTrainTracker:Line 2 northbound:       0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 1 0 1 0 0 0 0 0 0
// 00:01:30[I] LEDTrainTracker:Line 1 southbound: 0 0 2 1 1 0 0 0 0 1 0 0 0 0 2 0 1 0 0 0 0 0 0 1 0 0 0 3 0 0 0 0 0 0 1 0 1 1 1 1 0 0 1 0 0 0 0 1 0 0 1 0 1 0 0
// 00:01:30[I] LEDTrainTracker:Line 1 northbound: 0 1 0 0 0 0 1 0 0 0 0 0 1 0 0 0 1 0 1 0 0 0 0 0 0 0 2 0 0 1 0 0 1 0 1 0 1 0 1 0 0 0 1 0 0 0 2 0 1 0 1 0 4 0 0
void LEDTrainTracker::logTrainCounts() {
  // Log Line 2 Southbound LEDs (159 down to 135)
  String line2SouthboundLog = "Line 2 southbound:       ";
  for (int i = 159; i >= 135; i--) {
    int totalTrains = ledCounts[i].line1Count + ledCounts[i].line2Count;
    line2SouthboundLog += String(totalTrains);
    if (i > 135) {
      line2SouthboundLog += " ";
    }
  }
  LINK_LOGI(LOG_TAG, "%s", line2SouthboundLog.c_str());

  // Log Line 2 Northbound LEDs (110 up to 134)
  String line2NorthboundLog = "Line 2 northbound:       ";
  for (int i = 110; i <= 134; i++) {
    int totalTrains = ledCounts[i].line1Count + ledCounts[i].line2Count;
    line2NorthboundLog += String(totalTrains);
    if (i < 134) {
      line2NorthboundLog += " ";
    }
  }
  LINK_LOGI(LOG_TAG, "%s", line2NorthboundLog.c_str());

  // Log Line 1 southbound LEDs (109 down to 55)
  String line1SouthboundLog = "Line 1 southbound: ";
  for (int i = 109; i >= 55; i--) {
    int totalTrains = ledCounts[i].line1Count + ledCounts[i].line2Count;
    line1SouthboundLog += String(totalTrains);
    if (i > 55) {
      line1SouthboundLog += " ";
    }
  }
  LINK_LOGI(LOG_TAG, "%s", line1SouthboundLog.c_str());

  // Log Line 1 northbound LEDs (0 up to 54)
  String line1NorthboundLog = "Line 1 northbound: ";
  for (int i = 0; i <= 54; i++) {
    int totalTrains = ledCounts[i].line1Count + ledCounts[i].line2Count;
    line1NorthboundLog += String(totalTrains);
    if (i < 54) {
      line1NorthboundLog += " ";
    }
  }
  LINK_LOGI(LOG_TAG, "%s", line1NorthboundLog.c_str());
}
