#include "LEDController.h"
#include "TrainDataManager.h"
#include "LogManager.h"

static const char* TAG = "LEDController";

LEDController ledController;

static const RgbColor COLOR_BLUE = RgbColor(0, 0, 32);
static const RgbColor COLOR_GREEN = RgbColor(0, 32, 0);
static const RgbColor COLOR_YELLOW = RgbColor(32, 32, 0);
static const RgbColor COLOR_BLACK = RgbColor(0, 0, 0);

static const RgbColor LINE_1_COLOR = COLOR_GREEN;
static const RgbColor LINE_2_COLOR = COLOR_BLUE;

void LEDController::initializeStationMaps() {
  // Line 1 station mapping based on the reference implementation
  // Format: (northbound_index, southbound_index)
  // Indexes are origin 0.
  line1StationMap["Federal Way Downtown"] =    {1, 108};
  line1StationMap["Star Lake"] =               {3, 106};
  line1StationMap["Kent Des Moines"] =         {5, 104};
  line1StationMap["Angle Lake"] =              {7, 102};
  line1StationMap["SeaTac/Airport"] =          {9, 100};
  line1StationMap["Tukwila Int'l Blvd"] =      {11, 98};
  line1StationMap["Rainier Beach"] =           {13, 96};
  line1StationMap["Othello"] =                 {15, 94};
  line1StationMap["Columbia City"] =           {17, 92};
  line1StationMap["Mount Baker"] =             {19, 90};
  line1StationMap["Beacon Hill"] =             {21, 88};
  line1StationMap["SODO"] =                    {23, 86};
  line1StationMap["Stadium"] =                 {25, 84};
  line1StationMap["Int'l Dist/Chinatown"] =    {27, 82};
  line1StationMap["Pioneer Square"] =          {29, 80};
  line1StationMap["Symphony"] =                {31, 78};
  line1StationMap["Westlake"] =                {33, 76};
  line1StationMap["Capitol Hill"] =            {35, 74};
  line1StationMap["Univ of Washington"] =      {37, 72};
  line1StationMap["U District"] =              {39, 70};
  line1StationMap["Roosevelt"] =               {41, 68};
  line1StationMap["Northgate"] =               {43, 66};
  line1StationMap["Pinehurst"] =               {45, 64};
  line1StationMap["Shoreline South/148th"] =   {47, 62};
  line1StationMap["Shoreline North/185th"] =   {49, 60};
  line1StationMap["Mountlake Terrace"] =       {51, 58};
  line1StationMap["Lynnwood City Center"] =    {53, 56};
  
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
  setAllLEDs(COLOR_BLACK);
  strip.Show();

  LINK_LOGI(TAG, "LEDs initialized");
}

void LEDController::setAllLEDs(const RgbColor& color) {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.SetPixelColor(i, color);
  }
}

void LEDController::setTrainLED(int ledIndex, const RgbColor& color) {
  if (ledIndex >= 0 && ledIndex < LED_COUNT) {
    strip.SetPixelColor(ledIndex, color);
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
    auto closestStationInfo = line1StationMap.find(train.closestStopName);
    if (closestStationInfo == line1StationMap.end()) {
      LINK_LOGW(TAG, "Closest station '%s' not found in Line 1 map", train.closestStopName.c_str());
      return 0; // Default to lighting the first LED if station not found, to at least indicate presence of train.
    }

    // Get the LED data for the closest station
    const StationLEDMapping& closestMapping = closestStationInfo->second;

    ledIndex = isNorthbound ? closestMapping.northboundIndex : closestMapping.southboundIndex;
  }    
  // If train is moving between stations, light the LED that is one position closer to the next station.
  else if (train.state == TrainState::MOVING) {
    auto nextStationInfo = line1StationMap.find(train.nextStopName);
    if (nextStationInfo == line1StationMap.end()) {
      LINK_LOGW(TAG, "Next station '%s' not found in Line 1 map", train.nextStopName.c_str());
      return 0; // Default to lighting the first LED if station not found, to at least indicate presence of train.
    }

    const StationLEDMapping& nextMapping = nextStationInfo->second;
    if (isNorthbound) {
      // For northbound trains, the LED index should be one less than the next station's northbound index
      ledIndex = nextMapping.northboundIndex - 1;
    } else {
      // For southbound trains, the LED index should be one more than the next station's southbound index
      ledIndex = nextMapping.southboundIndex + 1;
    }    
  }

  // Ensure the index is within valid bounds
  if (ledIndex < 0 || ledIndex >= LED_COUNT) {
    LINK_LOGW(TAG, "LED index %d out of bounds for train %s", ledIndex, train.tripId.c_str());
  }

  return ledIndex;
}

void LEDController::displayTrainPositions() {
  // Set all LEDs to black in memory (without calling Show() to avoid flash)
  setAllLEDs(COLOR_BLACK);
  
  // Get train data from TrainDataManager
  const std::vector<TrainData>& trains = trainDataManager.getTrainDataList();
  
  // Process each train and set its LED
  for (const TrainData& train : trains) {
    int ledIndex = getTrainLEDIndex(train);
    
    if (ledIndex >= 0) {
      // Use green color for Line 1
      setTrainLED(ledIndex, train.state == TrainState::AT_STATION ? LINE_1_COLOR : COLOR_YELLOW);

      if (train.state == TrainState::AT_STATION) {
        LINK_LOGD(TAG, "Train %s at LED %d (closest: %s, state: AT_STATION, dir: %s)", 
                 train.tripId.c_str(), ledIndex, 
                 train.closestStopName.c_str(),
                 train.direction == TrainDirection::NORTHBOUND ? "Northbound" : "Southbound");
      } else {
        LINK_LOGD(TAG, "Train %s at LED %d (next: %s, state: MOVING, dir: %s)", 
                 train.tripId.c_str(), ledIndex, 
                 train.nextStopName.c_str(),
                 train.direction == TrainDirection::NORTHBOUND ? "Northbound" : "Southbound");
      }  
    }
  }
  
  // Update the LED strip once with all changes
  // Update the LED strip once with all changes
  strip.Show();
}
