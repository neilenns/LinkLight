#ifndef TRAINDATAMANAGER_H
#define TRAINDATAMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
// Only include the PSRAM components we need to avoid compilation issues with InMemoryFS
#include "esp32-psram/AllocatorPSRAM.h"
#include "esp32-psram/VectorPSRAM.h"

// Train state enum
enum class TrainState {
  AT_STATION,
  MOVING
};

// Train direction enum
enum class TrainDirection {
  SOUTHBOUND = 0,
  NORTHBOUND = 1
};

// Train data structure
struct TrainData {
  String closestStop;
  String closestStopName;
  int closestStopTimeOffset;
  String nextStop;
  String nextStopName;
  int nextStopTimeOffset;
  String tripId;
  TrainDirection direction;
  String routeId;
  String tripHeadsign;
  String line;  // Line identifier (e.g., "Line 1", "Line 2")
  TrainState state;  // Whether the train is at a station or moving between stations
};

class TrainDataManager {
public:
  void updateTrainPositions();
  
  // Returns the current list of train data parsed from the API or sample data
  const esp32_psram::VectorPSRAM<TrainData>& getTrainDataList() const { return trainDataList; }
  
private:
  bool parseTrainDataFromJson(JsonDocument& doc, const String& line);
  void fetchTrainDataForRoute(const String& routeId, const String& lineName, const String& apiKey);
  esp32_psram::VectorPSRAM<TrainData> trainDataList;
};

extern TrainDataManager trainDataManager;

#endif // TRAINDATAMANAGER_H
