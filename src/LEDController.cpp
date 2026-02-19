#include "LEDController.h"
#include "TrainDataManager.h"
#include "LogManager.h"
#include "colors.h"
#include "PreferencesManager.h"
#include <ArduinoJson.h>
#include "PSRAMJsonAllocator.h"

static const char* LOG_TAG = "LEDController";

// Row definitions shared by logTrainCounts() and serializeLEDState().
// The extra spaces in the Line 2 log prefixes are intentional and align the digits with how things look
// on the physical LED layout, with the between-station LEDs between Judkins Park and ID/Chinatown
// aligning above ID/Chinatown.
struct LEDRowDef {
  const char* label;
  const char* logPrefix;
  int start;
  int end;
  bool descending;
};

static const LEDRowDef LED_ROW_DEFS[] = {
  { "Line 2 southbound", "Line 2 southbound:       ", 159, 135, true  },
  { "Line 2 northbound", "Line 2 northbound:       ", 110, 134, false },
  { "Line 1 southbound", "Line 1 southbound: ",       109,  55, true  },
  { "Line 1 northbound", "Line 1 northbound: ",         0,  54, false },
};

LEDController ledController;

void LEDController::initializeStationMaps() {
  // Indexes are origin 0.
  // {northboundIndex, northboundEnrouteIndex, southboundIndex, southboundEnrouteIndex}
  // 1 and 2 Line stations
  stationMap["Lynnwood City Center"] =    {1, 0, 108, 107};
  stationMap["Mountlake Terrace"] =       {3, 2, 106, 105};
  stationMap["Shoreline North/185th"] =   {5, 4, 104, 103};
  stationMap["Shoreline South/148th"] =   {7, 6, 102, 101};
  stationMap["Pinehurst"] =               {9, 8, 100, 99};
  stationMap["Northgate"] =               {11, 10, 98, 97};
  stationMap["Roosevelt"] =               {13, 12, 96, 95};
  stationMap["U District"] =              {15, 14, 94, 93};
  stationMap["Univ of Washington"] =      {17, 16, 92, 91};
  stationMap["Capitol Hill"] =            {19, 18, 90, 89};
  stationMap["Westlake"] =                {21, 20, 88, 87};
  stationMap["Symphony"] =                {23, 22, 86, 85};
  stationMap["Pioneer Square"] =          {25, 24, 84, 83};
  stationMap["Int'l Dist/Chinatown"] =    {27, 26, 82, 81};

  // 1 Line stations
  stationMap["Stadium"] =                 {29, 28, 80, 79};
  stationMap["SODO"] =                    {31, 30, 78, 77};
  stationMap["Beacon Hill"] =             {33, 32, 76, 75};
  stationMap["Mount Baker"] =             {35, 34, 74, 73};
  stationMap["Columbia City"] =           {37, 36, 72, 71};
  stationMap["Othello"] =                 {39, 38, 70, 69};
  stationMap["Rainier Beach"] =           {41, 40, 68, 67};
  stationMap["Tukwila Int'l Blvd"] =      {43, 42, 66, 65};
  stationMap["SeaTac/Airport"] =          {45, 44, 64, 63};
  stationMap["Angle Lake"] =              {47, 46, 62, 61};
  stationMap["Kent Des Moines"] =         {49, 48, 60, 59};
  stationMap["Star Lake"] =               {51, 50, 58, 57};
  stationMap["Federal Way Downtown"] =    {53, 52, 56, 55};

  // 2 Line stations
  stationMap["Downtown Redmond"] =        {111, 110, 158, 157};
  stationMap["Marymoor Village"] =        {113, 112, 156, 155};
  stationMap["Redmond Technology"] =      {115, 114, 154, 153};
  stationMap["Overlake Village"] =        {117, 116, 152, 151};
  stationMap["BelRed"] =                  {119, 118, 150, 149};
  stationMap["Spring District"] =         {121, 120, 148, 147};
  stationMap["Wilburton"] =               {123, 122, 146, 145};
  stationMap["Bellevue Downtown"] =       {125, 124, 144, 143};
  stationMap["East Main"] =               {127, 126, 142, 141};
  stationMap["South Bellevue"] =          {129, 128, 140, 139};
  stationMap["Mercer Island"] =           {131, 130, 138, 137};
  stationMap["Judkins Park"] =            {133, 132, 136, 135};
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

int LEDController::getTrainLEDIndex(const TrainData& train) const {
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
    ledIndex = isNorthbound ? nextMapping.northboundEnrouteIndex : nextMapping.southboundEnrouteIndex;
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
      // Record this train at its LED index
      trainTracker.addTrain(ledIndex, train.line, train.vehicleId);
      
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
  logTrainCounts();
  
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

  if (mapping.northboundEnrouteIndex >= 0 && mapping.northboundEnrouteIndex < LED_COUNT) {
    strip.SetPixelColor(mapping.northboundEnrouteIndex, COLOR_YELLOW);
    LINK_LOGD(LOG_TAG, "Northbound enroute LED at index %d", mapping.northboundEnrouteIndex);
  }

  if (mapping.southboundIndex >= 0 && mapping.southboundIndex < LED_COUNT) {
    strip.SetPixelColor(mapping.southboundIndex, COLOR_BLUE);
    LINK_LOGD(LOG_TAG, "Southbound LED at index %d", mapping.southboundIndex);
  }

  if (mapping.southboundEnrouteIndex >= 0 && mapping.southboundEnrouteIndex < LED_COUNT) {
    strip.SetPixelColor(mapping.southboundEnrouteIndex, COLOR_YELLOW);
    LINK_LOGD(LOG_TAG, "Southbound enroute LED at index %d", mapping.southboundEnrouteIndex);
  }

  // Update the strip
  strip.Show();
}

const std::map<String, StationLEDMapping>& LEDController::getStationMap() const {
  return stationMap;
}

// Logs the train counts in four rows, one for each LED segment of the physical layout.
// 00:01:30[I] LEDController:Line 2 southbound:       0 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 1 0 0 0 1 0 0 0 0
// 00:01:30[I] LEDController:Line 2 northbound:       0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 1 0 1 0 0 0 0 0 0
// 00:01:30[I] LEDController:Line 1 southbound: 0 0 2 1 1 0 0 0 0 1 0 0 0 0 2 0 1 0 0 0 0 0 0 1 0 0 0 3 0 0 0 0 0 0 1 0 1 1 1 1 0 0 1 0 0 0 0 1 0 0 1 0 1 0 0
// 00:01:30[I] LEDController:Line 1 northbound: 0 1 0 0 0 0 1 0 0 0 0 0 1 0 0 0 1 0 1 0 0 0 0 0 0 0 2 0 0 1 0 0 1 0 1 0 1 0 1 0 0 0 1 0 0 0 2 0 1 0 1 0 4 0 0
void LEDController::logTrainCounts() const {
  // Iterate over all four physical LED rows and log the count of trains at each LED position.
  for (const LEDRowDef& row : LED_ROW_DEFS) {
    String log = row.logPrefix;

    // Walk the LEDs in the correct direction for this row (descending or ascending index).
    int step = row.descending ? -1 : 1;
    for (int ledIndex = row.start; row.descending ? (ledIndex >= row.end) : (ledIndex <= row.end); ledIndex += step) {
      log += String(trainTracker.getTrainsAtLED(ledIndex).size());
      if (ledIndex != row.end) {
        log += " ";
      }
    }

    LINK_LOGI(LOG_TAG, "%s", log.c_str());
  }
}

void LEDController::serializeLEDState(String& output) const {
  JsonDocument doc(PSRAMJsonAllocator::instance());
  doc["type"] = "leds";

  // Build the 4 rows of LED data. Each LED carries only its vehicleIds;
  // the browser derives the colour from its existing train data.
  JsonArray rows = doc["rows"].to<JsonArray>();

  // Iterate over the four physical LED rows and build the JSON representation.
  for (const LEDRowDef& row : LED_ROW_DEFS) {
    JsonObject rowObj = rows.add<JsonObject>();
    rowObj["label"] = row.label;

    // Include the total LED count so the browser knows how many squares to render,
    // plus the start index and direction so it can compute physical LED indices.
    int count = row.descending ? (row.start - row.end + 1) : (row.end - row.start + 1);
    rowObj["count"] = count;
    rowObj["start"] = row.start;
    rowObj["descending"] = row.descending;

    // Only emit LEDs that have at least one train — empty LEDs are implicitly off.
    JsonArray leds = rowObj["leds"].to<JsonArray>();
    int step = row.descending ? -1 : 1;
    for (int ledIndex = row.start; row.descending ? (ledIndex >= row.end) : (ledIndex <= row.end); ledIndex += step) {
      const std::vector<TrainAtLED>& trains = trainTracker.getTrainsAtLED(ledIndex);
      if (trains.empty()) {
        continue;
      }

      JsonObject led = leds.add<JsonObject>();
      led["index"] = ledIndex;

      // Add the vehicleId for each train present at this LED position.
      JsonArray trainIds = led["trainIds"].to<JsonArray>();
      for (const TrainAtLED& train : trains) {
        trainIds.add(train.vehicleId);
      }
    }
  }

  serializeJson(doc, output);
}
