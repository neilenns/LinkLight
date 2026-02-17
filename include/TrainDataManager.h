#ifndef TRAINDATAMANAGER_H
#define TRAINDATAMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

// Train data structure
struct TrainData {
  String closestStop;
  String closestStopName;
  int closestStopTimeOffset;
  String nextStop;
  String nextStopName;
  int nextStopTimeOffset;
  String tripId;
  String directionId;
  String routeId;
  String tripHeadsign;
  String line;  // Line identifier (e.g., "Line 1", "Line 2")
};

class TrainDataManager {
public:
  void updateTrainPositions();
  
  // Returns the current list of train data parsed from the API or sample data
  const std::vector<TrainData>& getTrainDataList() const { return trainDataList; }
  
private:
  bool parseTrainDataFromJson(JsonDocument& doc, const String& line);
  std::vector<TrainData> trainDataList;
};

extern TrainDataManager trainDataManager;

#endif // TRAINDATAMANAGER_H
