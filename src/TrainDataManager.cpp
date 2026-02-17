#include "TrainDataManager.h"
#include <HTTPClient.h>
#include <LittleFS.h>
#include <esp_log.h>
#include <map>
#include "config.h"
#include "PreferencesManager.h"

static const char* TAG = "TrainDataManager";
static const float MIN_DISTANCE_THRESHOLD = 0.001f;

TrainDataManager trainDataManager;

// Trip information structure
struct TripInfo {
  String directionId;
  String routeId;
  String tripHeadsign;
};

bool TrainDataManager::parseTrainDataFromJson(JsonDocument& doc) {
  // Clear previous train data
  trainDataList.clear();

  // Get the data object
  JsonObject data = doc["data"];
  if (data.isNull()) {
    ESP_LOGW(TAG, "JSON response missing 'data' object");
    return false;
  }

  // First, build a map of trip information
  std::map<String, TripInfo> tripMap;
  JsonArray trips = data["references"]["trips"];
  if (!trips.isNull()) {
    for (JsonObject trip : trips) {
      String tripId = trip["id"].as<String>();
      TripInfo info;
      info.directionId = trip["directionId"].as<String>();
      info.routeId = trip["routeId"].as<String>();
      info.tripHeadsign = trip["tripHeadsign"].as<String>();
      tripMap[tripId] = info;
    }
    ESP_LOGI(TAG, "Loaded %d trip references", tripMap.size());
  }

  // Build a map of stop IDs to stop names
  std::map<String, String> stopsToNames;
  JsonArray stops = data["references"]["stops"];
  if (!stops.isNull()) {
    for (JsonObject stop : stops) {
      String stopId = stop["id"].as<String>();
      String stopName = stop["name"].as<String>();
      stopsToNames[stopId] = stopName;
    }
    ESP_LOGI(TAG, "Loaded %d stop references", stopsToNames.size());
  }

  // Process the list array
  JsonArray list = data["list"];
  if (list.isNull()) {
    ESP_LOGW(TAG, "JSON response missing 'data.list' array");
    return false;
  }

  for (JsonObject item : list) {
    TrainData train;

    // Extract tripId from the list item
    train.tripId = item["tripId"].as<String>();

    // Validate critical fields - skip trains with missing data
    // Note: We skip trains missing critical position/timing data (status, nextStop, nextStopTimeOffset)
    // but allow trains that haven't started yet (scheduledDistanceAlongTrip == 0)
    JsonObject status = item["status"];
    if (status.isNull()) {
      ESP_LOGW(TAG, "Status missing for trip %s", train.tripId.c_str());
      continue;
    }

    // Check for nextStop
    if (status["nextStop"].isNull()) {
      ESP_LOGW(TAG, "No next stop for trip %s", train.tripId.c_str());
      continue;
    }
    train.nextStop = status["nextStop"].as<String>();

    // Check for nextStopTimeOffset
    if (status["nextStopTimeOffset"].isNull()) {
      ESP_LOGW(TAG, "No next stop time offset for trip %s", train.tripId.c_str());
      continue;
    }
    train.nextStopTimeOffset = status["nextStopTimeOffset"].as<int>();

    // Extract closestStop and offset
    train.closestStop = status["closestStop"].as<String>();
    train.closestStopTimeOffset = status["closestStopTimeOffset"].as<int>();

    // Check if trip is in progress
    // Note: We log warnings but don't skip trains that haven't started yet,
    // as they are still valid and should be displayed
    if (!status["scheduledDistanceAlongTrip"].isNull()) {
      float schedDist = status["scheduledDistanceAlongTrip"].as<float>();
      if (schedDist < MIN_DISTANCE_THRESHOLD) {
        ESP_LOGW(TAG, "Trip %s not in progress yet, scheduledDistanceAlongTrip: %.2f", 
                 train.tripId.c_str(), schedDist);
      }
    } else {
      ESP_LOGW(TAG, "Trip %s not in progress yet, no scheduledDistanceAlongTrip", 
               train.tripId.c_str());
    }

    // Look up stop names from the stops map
    if (stopsToNames.find(train.closestStop) != stopsToNames.end()) {
      train.closestStopName = stopsToNames[train.closestStop];
    } else {
      train.closestStopName = train.closestStop; // Fall back to ID if name not found
    }

    if (stopsToNames.find(train.nextStop) != stopsToNames.end()) {
      train.nextStopName = stopsToNames[train.nextStop];
    } else {
      train.nextStopName = train.nextStop; // Fall back to ID if name not found
    }

    // Merge trip information if available
    if (tripMap.find(train.tripId) != tripMap.end()) {
      TripInfo& tripInfo = tripMap[train.tripId];
      train.directionId = tripInfo.directionId;
      train.routeId = tripInfo.routeId;
      train.tripHeadsign = tripInfo.tripHeadsign;
    }

    // Add to the list
    trainDataList.push_back(train);

    // Log parsed data
    ESP_LOGI(TAG, "Train: tripId=%s, closestStop=%s (%s), closestStopOffset=%d, nextStop=%s (%s), nextStopOffset=%d, direction=%s, route=%s, headsign=%s",
      train.tripId.c_str(),
      train.closestStop.c_str(),
      train.closestStopName.c_str(),
      train.closestStopTimeOffset,
      train.nextStop.c_str(),
      train.nextStopName.c_str(),
      train.nextStopTimeOffset,
      train.directionId.c_str(),
      train.routeId.c_str(),
      train.tripHeadsign.c_str());
  }

  ESP_LOGI(TAG, "Processed %d train positions", trainDataList.size());
  return true;
}

void TrainDataManager::updateTrainPositions() {
  static const char* SAMPLE_DATA_PATH = "/data.json";

  String apiKey = preferencesManager.getApiKey();
  String routeId = preferencesManager.getRouteId();

  if (apiKey.isEmpty()) {
    ESP_LOGW(TAG, "API key not configured, loading sample data from %s", SAMPLE_DATA_PATH);

    File sampleFile = LittleFS.open(SAMPLE_DATA_PATH, "r");
    if (!sampleFile) {
      ESP_LOGE(TAG, "Sample data not found.");
      return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, sampleFile);
    sampleFile.close();
    if (error) {
      ESP_LOGE(TAG, "Sample JSON parsing failed: %s", error.c_str());
      return;
    }

    if (parseTrainDataFromJson(doc)) {
      ESP_LOGI(TAG, "Successfully loaded sample train data from LittleFS");
    }

    return;
  }

  ESP_LOGI(TAG, "Updating train positions...");

  HTTPClient http;
  String url = String(API_BASE_URL) + "/trips-for-route/" + routeId + ".json?" + API_KEY_PARAM + "=" + apiKey;

  http.setTimeout(10000);
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    WiFiClient* stream = http.getStreamPtr();

    if (stream == nullptr) {
      ESP_LOGE(TAG, "Failed to get HTTP stream");
      http.end();
      return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, *stream);

    if (error) {
      ESP_LOGE(TAG, "JSON parsing failed: %s", error.c_str());
    } else {
      ESP_LOGI(TAG, "Successfully retrieved train data");
      parseTrainDataFromJson(doc);
    }
  } else {
    ESP_LOGW(TAG, "HTTP request failed: %d", httpCode);
  }

  http.end();
}
