#include "LEDController.h"
#include "TrainDataManager.h"
#include <esp_log.h>

// Thresholds for determining if a train is in transit between stations
#define MIN_DEPARTED_TIME_SECONDS 30   // Min seconds since leaving closest station
#define MAX_ARRIVAL_TIME_SECONDS 180   // Max seconds until arriving at next station

static const char* TAG = "LEDController";

LEDController ledController;

// Green color for Line 1 trains
static const RgbColor LINE_1_COLOR = RgbColor(0, 255, 0);

void LEDController::initializeStationMaps() {
  // Line 1 station mapping based on the reference implementation
  // Format: (southbound_index, leds_from_prev_station), (northbound_index, leds_from_prev_station)
  line1StationMap["Federal Way Downtown"] =    {1, 4, 209, 1};
  line1StationMap["Star Lake"] =               {6, 3, 204, 4};
  line1StationMap["Kent Des Moines"] =         {10, 6, 200, 3};
  line1StationMap["Angle Lake"] =              {17, 5, 193, 6};
  line1StationMap["SeaTac/Airport"] =          {23, 7, 187, 5};
  line1StationMap["Tukwila Int'l Blvd"] =      {31, 4, 182, 4};
  line1StationMap["Rainier Beach"] =           {36, 2, 173, 8};
  line1StationMap["Othello"] =                 {39, 2, 170, 2};
  line1StationMap["Columbia City"] =           {42, 2, 167, 2};
  line1StationMap["Mount Baker"] =             {45, 1, 164, 2};
  line1StationMap["Beacon Hill"] =             {47, 3, 159, 4};
  line1StationMap["SODO"] =                    {51, 1, 157, 1};
  line1StationMap["Stadium"] =                 {53, 2, 155, 1};
  line1StationMap["Int'l Dist/Chinatown"] =    {56, 1, 152, 2};
  line1StationMap["Pioneer Square"] =          {58, 1, 150, 1};
  line1StationMap["Symphony"] =                {60, 1, 148, 1};
  line1StationMap["Westlake"] =                {62, 3, 146, 1};
  line1StationMap["Capitol Hill"] =            {66, 3, 143, 2};
  line1StationMap["Univ of Washington"] =      {70, 2, 137, 5};
  line1StationMap["U District"] =              {73, 4, 133, 3};
  line1StationMap["Roosevelt"] =               {78, 5, 129, 3};
  line1StationMap["Northgate"] =               {84, 4, 123, 5};
  line1StationMap["Pinehurst"] =               {89, 1, 118, 4};
  line1StationMap["Shoreline South/148th"] =   {91, 2, 116, 1};
  line1StationMap["Shoreline North/185th"] =   {94, 4, 113, 2};
  line1StationMap["Mountlake Terrace"] =       {99, 3, 110, 2};
  line1StationMap["Lynnwood City Center"] =    {103, 1, 106, 3};
  
  ESP_LOGI(TAG, "Initialized Line 1 station map with %d stations", line1StationMap.size());
}

void LEDController::setup() {
  ESP_LOGI(TAG, "Setting up LEDs...");
  
  strip.Begin();
  strip.Show(); // Initialize all pixels to 'off'
  
  // Initialize station maps
  initializeStationMaps();
  
  // Basic startup self-test: full-strip RGB flashes
  const RgbColor testColors[] = {
    RgbColor(32, 0, 0),
    RgbColor(0, 32, 0),
    RgbColor(0, 0, 32)
  };

  for (const auto& color : testColors) {
    for (int i = 0; i < LED_COUNT; i++) {
      strip.SetPixelColor(i, color);
    }
    strip.Show();
    delay(2000);
  }

  // Return LEDs to off state after test
  clearAllLEDs();
  
  ESP_LOGI(TAG, "LEDs initialized");
}

void LEDController::clearAllLEDs() {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.SetPixelColor(i, RgbColor(0, 0, 0));
  }
  strip.Show();
}

void LEDController::setTrainLED(int ledIndex, const RgbColor& color) {
  if (ledIndex >= 0 && ledIndex < LED_COUNT) {
    strip.SetPixelColor(ledIndex, color);
  }
}

int LEDController::getTrainLEDIndex(const String& line, const String& direction, 
                                     const String& closestStation, const String& nextStation, 
                                     int closestStopTimeOffset, int nextStopTimeOffset) {
  // Only handle Line 1 for now
  if (line != LINE_1_NAME) {
    return -1;
  }
  
  // Determine if train is northbound (direction "1") or southbound (direction "0")
  bool isNorthbound = (direction == "1");
  
  // Check if closest station is in our map
  auto closestIt = line1StationMap.find(closestStation);
  if (closestIt == line1StationMap.end()) {
    ESP_LOGW(TAG, "Closest station '%s' not found in Line 1 map", closestStation.c_str());
    return -1;
  }
  
  // Check if next station is in our map
  auto nextIt = line1StationMap.find(nextStation);
  if (nextIt == line1StationMap.end()) {
    ESP_LOGW(TAG, "Next station '%s' not found in Line 1 map", nextStation.c_str());
    return -1;
  }
  
  const StationLEDMapping& closestMapping = closestIt->second;
  const StationLEDMapping& nextMapping = nextIt->second;
  
  int stationIndex;
  int nextStationIndex;
  int ledsToNext;
  
  if (isNorthbound) {
    stationIndex = closestMapping.northboundIndex;
    nextStationIndex = nextMapping.northboundIndex;
    ledsToNext = nextMapping.northboundLedsFromPrev;
  } else {
    stationIndex = closestMapping.southboundIndex;
    nextStationIndex = nextMapping.southboundIndex;
    ledsToNext = nextMapping.southboundLedsFromPrev;
  }
  
  // Determine train position based on time offsets
  // If the train has departed the closest station and is approaching the next station,
  // calculate its position between stations
  if (closestStopTimeOffset < -MIN_DEPARTED_TIME_SECONDS && 
      nextStopTimeOffset > 0 && 
      nextStopTimeOffset < MAX_ARRIVAL_TIME_SECONDS) {
    // Train is in transit between stations
    int totalTime = abs(closestStopTimeOffset) + nextStopTimeOffset;
    
    // Calculate progress from closest station to next station (0.0 to 1.0)
    float progress = (float)abs(closestStopTimeOffset) / (float)totalTime;
    
    // Calculate LED offset based on progress and available LEDs between stations
    // Note: Both northbound and southbound sections have decreasing indices as trains move
    int ledOffset = 0;
    if (totalTime > 0 && ledsToNext > 1) {
      ledOffset = (int)(progress * (ledsToNext - 1));
    }
    
    // Both directions use decreasing indices, so we subtract the offset
    return stationIndex - ledOffset;
  }
  
  // Default: train is at or very close to the closest station
  return stationIndex;
}

void LEDController::displayTrainPositions() {
  // Clear all LEDs first
  clearAllLEDs();
  
  // Get train data from TrainDataManager
  const std::vector<TrainData>& trains = trainDataManager.getTrainDataList();
  
  // Process each train and set its LED
  for (const TrainData& train : trains) {
    int ledIndex = getTrainLEDIndex(train.line, train.directionId, 
                                     train.closestStopName, train.nextStopName,
                                     train.closestStopTimeOffset, train.nextStopTimeOffset);
    
    if (ledIndex >= 0) {
      // Use green color for Line 1
      setTrainLED(ledIndex, LINE_1_COLOR);
      
      ESP_LOGD(TAG, "Train %s at LED %d (closest: %s, next: %s, dir: %s)", 
               train.tripId.c_str(), ledIndex, 
               train.closestStopName.c_str(), train.nextStopName.c_str(),
               train.directionId.c_str());
    }
  }
  
  // Update the LED strip
  strip.Show();
}
