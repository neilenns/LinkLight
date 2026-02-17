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
};

class TrainDataManager {
public:
  void updateTrainPositions();
  
  // Returns the current list of train data parsed from the API or sample data
  const std::vector<TrainData>& getTrainDataList() const { return trainDataList; }
  
private:
  bool parseTrainDataFromJson(JsonDocument& doc);
  std::vector<TrainData> trainDataList;
};

extern TrainDataManager trainDataManager;

#endif // TRAINDATAMANAGER_H
