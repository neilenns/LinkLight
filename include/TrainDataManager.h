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

// Line identifier enum
enum class Line {
  LINE_1 = 1,
  LINE_2 = 2
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
  String vehicleId;
  TrainDirection direction;
  String routeId;
  String tripHeadsign;
  Line line;  // Line identifier
  TrainState state;  // Whether the train is at a station or moving between stations
};

class TrainDataManager {
public:
  void updateTrainPositions();
  
  // Returns the current list of train data parsed from the API or sample data
  const esp32_psram::VectorPSRAM<TrainData>& getTrainDataList() const { return trainDataList; }

  // Serializes the current train data list to a JSON string
  void getTrainDataAsJson(String& output) const;
  
private:
  bool parseTrainDataFromJson(JsonDocument& doc, Line line);
  void fetchTrainDataForRoute(const String& routeId, Line line, const String& apiKey);
  void buildTrainJsonObject(JsonObject trainObj, const TrainData& train) const;
  esp32_psram::VectorPSRAM<TrainData> trainDataList;
};

extern TrainDataManager trainDataManager;

#endif // TRAINDATAMANAGER_H
