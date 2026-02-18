#include "LEDController.h"
#include "TrainDataManager.h"
#include "LogManager.h"
#include "colors.h"
#include "PreferencesManager.h"

static const char* LOG_TAG = "LEDController";

LEDController ledController;

void LEDController::initializeStationMaps() {
  // Indexes are origin 0.
  stationMap["Federal Way Downtown"] =    {1, 108};
  stationMap["Star Lake"] =               {3, 106};
  stationMap["Kent Des Moines"] =         {5, 104};
  stationMap["Angle Lake"] =              {7, 102};
  stationMap["SeaTac/Airport"] =          {9, 100};
  stationMap["Tukwila Int'l Blvd"] =      {11, 98};
  stationMap["Rainier Beach"] =           {13, 96};
  stationMap["Othello"] =                 {15, 94};
  stationMap["Columbia City"] =           {17, 92};
  stationMap["Mount Baker"] =             {19, 90};
  stationMap["Beacon Hill"] =             {21, 88};
  stationMap["SODO"] =                    {23, 86};
  stationMap["Stadium"] =                 {25, 84};
  stationMap["Int'l Dist/Chinatown"] =    {27, 82};
  stationMap["Pioneer Square"] =          {29, 80};
  stationMap["Symphony"] =                {31, 78};
  stationMap["Westlake"] =                {33, 76};
  stationMap["Capitol Hill"] =            {35, 74};
  stationMap["Univ of Washington"] =      {37, 72};
  stationMap["U District"] =              {39, 70};
  stationMap["Roosevelt"] =               {41, 68};
  stationMap["Northgate"] =               {43, 66};
  stationMap["Pinehurst"] =               {45, 64};
  stationMap["Shoreline South/148th"] =   {47, 62};
  stationMap["Shoreline North/185th"] =   {49, 60};
  stationMap["Mountlake Terrace"] =       {51, 58};
  stationMap["Lynnwood City Center"] =    {53, 56};
  stationMap["Downtown Redmond"] =        {111, 158};
  stationMap["Marymoor Village"] =        {113, 156};
  stationMap["Redmond Technology"] =      {115, 154};
  stationMap["Overlake Village"] =        {117, 152};
  stationMap["BelRed"] =                  {119, 150};
  stationMap["Spring District"] =         {121, 148};
  stationMap["Wilburton"] =               {123, 146};
  stationMap["Bellevue Downtown"] =       {125, 144};
  stationMap["East Main"] =               {127, 142};
  stationMap["South Bellevue"] =          {129, 140};
  stationMap["Mercer Island"] =           {131, 138};
  stationMap["Judkins Park"] =            {133, 136};
}

void LEDController::setup() {
  LINK_LOGD(LOG_TAG, "Setting up LEDs...");
  
  strip.Begin();
  strip.Show(); // Initialize all pixels to 'off'
  
  // Initialize station maps
  initializeStationMaps();
}

void LEDController::startupAnimation() {
  const int delayMs = 20;  // Delay between each color

  // Flash each LED blue
  for (int i = 0; i < LED_COUNT; i++) {
    // Show blue
    strip.SetPixelColor(i, COLOR_BLUE);
    strip.Show();
    delay(delayMs);
    strip.SetPixelColor(i, COLOR_BLACK);
    strip.Show();
  }

  // Flash each LED green in the opposite direction
  for (int i = LED_COUNT -1; i >= 0; i--) {
    strip.SetPixelColor(i, COLOR_GREEN);
    strip.Show();
    delay(delayMs);
    strip.SetPixelColor(i, COLOR_BLACK);
    strip.Show();
  }

  // Turn all LEDs off at the end of the self-test
  setAllLEDs(COLOR_BLACK);
  strip.Show();

  LINK_LOGD(LOG_TAG, "LEDs initialized");
}

void LEDController::setAllLEDs(const RgbColor& color) {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.SetPixelColor(i, color);
  }
}

int LEDController::getTrainLEDIndex(const TrainData& train) {
  int ledIndex = -1;

  // Determine if train is northbound or southbound
  bool isNorthbound = (train.direction == TrainDirection::NORTHBOUND);

  // Start by processing trains at stations. The mapping logic is simple: figure out what the closest station is,
  // find it in the station map, then light the corresponding LED based on the direction of travel.
  if (train.state == TrainState::AT_STATION)
  {
    auto closestStationInfo = stationMap.find(train.closestStopName);
    if (closestStationInfo == stationMap.end()) {
      LINK_LOGW(LOG_TAG, "Closest station '%s' not found in Line 1 map", train.closestStopName.c_str());
      return 0; // Default to lighting the first LED if station not found, to at least indicate presence of train.
    }

    // Get the LED data for the closest station
    const StationLEDMapping& closestMapping = closestStationInfo->second;

    ledIndex = isNorthbound ? closestMapping.northboundIndex : closestMapping.southboundIndex;
  }    
  // If train is moving between stations, light the LED that is one position closer to the next station.
  else if (train.state == TrainState::MOVING) {
    auto nextStationInfo = stationMap.find(train.nextStopName);
    if (nextStationInfo == stationMap.end()) {
      LINK_LOGW(LOG_TAG, "Next station '%s' not found in Line 1 map", train.nextStopName.c_str());
      return 0; // Default to lighting the first LED if station not found, to at least indicate presence of train.
    }

    const StationLEDMapping& nextMapping = nextStationInfo->second;
    if (isNorthbound) {
      ledIndex = nextMapping.northboundIndex - 1;
    } else {
      ledIndex = nextMapping.southboundIndex - 1;
    }    
  }

  // Ensure the index is within valid bounds
  if (ledIndex < 0 || ledIndex >= LED_COUNT) {
    LINK_LOGW(LOG_TAG, "LED index %d out of bounds for vehicle %s", ledIndex, train.vehicleId.c_str());
    ledIndex = 0; // Default to first LED if out of bounds, to at least indicate presence of train.
  }

  return ledIndex;
}

void LEDController::displayTrainPositions() {
  // Reset train counts
  trainTracker.reset();
  
  // Get train data from TrainDataManager
  const esp32_psram::VectorPSRAM<TrainData>& trains = trainDataManager.getTrainDataList();
  
  // Get focused vehicle ID (if any)
  String focusedVehicleId = preferencesManager.getFocusedVehicleId();
  
  // Process each train and update the tracker
  for (const TrainData& train : trains) {
    // If a focused train is set, skip all other trains
    if (!focusedVehicleId.isEmpty() && train.vehicleId != focusedVehicleId) {
      continue;
    }
    
    int ledIndex = getTrainLEDIndex(train);
    
    if (ledIndex >= 0) {
      // Increment the count for this train's line at this LED
      trainTracker.incrementTrainCount(ledIndex, train.line);
      
      LINK_LOGD(LOG_TAG, "Train %s at LED %d (closest: %s, next: %s, state: %s, dir: %s, line: %d)", 
                train.vehicleId.c_str(), ledIndex, 
                train.closestStopName.c_str(),
                train.nextStopName.c_str(),
              train.state == TrainState::AT_STATION ? "AT_STATION" : "MOVING",
                train.direction == TrainDirection::NORTHBOUND ? "Northbound" : "Southbound",
                static_cast<int>(train.line));
    }
  }
  
  // Log train counts for debugging
  trainTracker.logTrainCounts();
  
  // Display all trains on the LED strip
  trainTracker.display(strip);
}

void LEDController::testStationLEDs(const String& stationName) {
  LINK_LOGI(LOG_TAG, "Testing LEDs for station: %s", stationName.c_str());
  
  // Find the station in the map
  auto stationInfo = stationMap.find(stationName);
  if (stationInfo == stationMap.end()) {
    LINK_LOGW(LOG_TAG, "Station '%s' not found in station map", stationName.c_str());
    return;
  }
  
  // Turn off all LEDs first
  setAllLEDs(COLOR_BLACK);
  
  // Get the LED indices for this station
  const StationLEDMapping& mapping = stationInfo->second;
  
  // Light up the northbound and southbound LEDs
  if (mapping.northboundIndex >= 0 && mapping.northboundIndex < LED_COUNT) {
    strip.SetPixelColor(mapping.northboundIndex, COLOR_GREEN);
    LINK_LOGD(LOG_TAG, "Northbound LED at index %d", mapping.northboundIndex);
  }
  
  if (mapping.southboundIndex >= 0 && mapping.southboundIndex < LED_COUNT) {
    strip.SetPixelColor(mapping.southboundIndex, COLOR_BLUE);
    LINK_LOGD(LOG_TAG, "Southbound LED at index %d", mapping.southboundIndex);
  }
  
  // Update the strip
  strip.Show();
}

const std::map<String, StationLEDMapping>& LEDController::getStationMap() const {
  return stationMap;
}
