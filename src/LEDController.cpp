#include "LEDController.h"
#include "TrainDataManager.h"
#include "LogManager.h"

static const char* TAG = "LEDController";

LEDController ledController;

static const RgbColor COLOR_BLUE = RgbColor(0, 0, 32);
static const RgbColor COLOR_GREEN = RgbColor(0, 32, 0);
static const RgbColor COLOR_BLACK = RgbColor(0, 0, 0);

static const RgbColor LINE_1_COLOR = COLOR_GREEN;
static const RgbColor LINE_2_COLOR = COLOR_BLUE;

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
  
  LINK_LOGI(TAG, "Initialized Line 1 station map with %d stations", line1StationMap.size());
}

void LEDController::setup() {
  LINK_LOGI(TAG, "Setting up LEDs...");
  
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
  clearAllLEDs();

  LINK_LOGI(TAG, "LEDs initialized");
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

int LEDController::getTrainLEDIndex(const TrainData& train) {
  // Only handle Line 1 for now
  if (train.line != LINE_1_NAME) {
    return -1;
  }
  
  // Determine if train is northbound (direction "1") or southbound (direction "0")
  bool isNorthbound = (train.directionId == "1");
  
  // Check if closest station is in our map
  auto closestIt = line1StationMap.find(train.closestStopName);
  if (closestIt == line1StationMap.end()) {
    LINK_LOGW(TAG, "Closest station '%s' not found in Line 1 map", train.closestStopName.c_str());
    return -1;
  }
  
  const StationLEDMapping& closestMapping = closestIt->second;
  
  int stationIndex;
  
  if (isNorthbound) {
    stationIndex = closestMapping.northboundIndex;
  } else {
    stationIndex = closestMapping.southboundIndex;
  }
  
  // If train is moving between stations, light the LED between closest and next station
  if (train.state == TrainState::MOVING) {
    // The in-between LED is one position closer to the next station
    // Both directions have decreasing indices, so subtract 1
    int betweenIndex = stationIndex - 1;
    
    // Ensure the index is within valid bounds
    if (betweenIndex < 0 || betweenIndex >= LED_COUNT) {
      LINK_LOGW(TAG, "In-between LED index %d out of bounds for train %s, using station index %d", 
               betweenIndex, train.tripId.c_str(), stationIndex);
      return stationIndex;
    }
    
    return betweenIndex;
  }
  
  // Train is at the station
  return stationIndex;
}

void LEDController::displayTrainPositions() {
  // Clear all LEDs first
  clearAllLEDs();
  
  // Get train data from TrainDataManager
  const std::vector<TrainData>& trains = trainDataManager.getTrainDataList();
  
  // Process each train and set its LED
  for (const TrainData& train : trains) {
    int ledIndex = getTrainLEDIndex(train);
    
    if (ledIndex >= 0) {
      // Use green color for Line 1
      setTrainLED(ledIndex, LINE_1_COLOR);
      
      LINK_LOGD(TAG, "Train %s at LED %d (closest: %s, state: %s, dir: %s)", 
               train.tripId.c_str(), ledIndex, 
               train.closestStopName.c_str(),
               train.state == TrainState::AT_STATION ? "AT_STATION" : "MOVING",
               train.directionId.c_str());
    }
  }
  
  // Update the LED strip
  strip.Show();
}
