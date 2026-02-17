#include "TrainDataManager.h"
#include <HTTPClient.h>
#include <LittleFS.h>
#include <esp_log.h>
#include <map>
#include "config.h"
#include "PreferencesManager.h"

static const char* TAG = "TrainDataManager";
static const float MIN_SCHEDULED_DISTANCE_THRESHOLD = 0.001f;

TrainDataManager trainDataManager;

// Trip information structure
struct TripInfo {
  String directionId;
  String routeId;
  String tripHeadsign;
};

bool TrainDataManager::parseTrainDataFromJson(JsonDocument& doc, const String& line) {
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
  std::map<String, String> stopIdToNameMap;
  JsonArray stops = data["references"]["stops"];
  if (!stops.isNull()) {
    for (JsonObject stop : stops) {
      String stopId = stop["id"].as<String>();
      String stopName = stop["name"].as<String>();
      stopIdToNameMap[stopId] = stopName;
    }
    ESP_LOGI(TAG, "Loaded %d stop references", stopIdToNameMap.size());
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
      float scheduledDistance = status["scheduledDistanceAlongTrip"].as<float>();
      if (scheduledDistance < MIN_SCHEDULED_DISTANCE_THRESHOLD) {
        ESP_LOGW(TAG, "Trip %s not in progress yet, scheduledDistanceAlongTrip: %.2f", 
                 train.tripId.c_str(), scheduledDistance);
      }
    } else {
      ESP_LOGW(TAG, "Trip %s not in progress yet, no scheduledDistanceAlongTrip", 
               train.tripId.c_str());
    }

    // Look up stop names from the stops map
    auto closestStopIt = stopIdToNameMap.find(train.closestStop);
    if (closestStopIt != stopIdToNameMap.end()) {
      train.closestStopName = closestStopIt->second;
    } else {
      ESP_LOGW(TAG, "Stop name not found for closestStop ID: %s", train.closestStop.c_str());
      train.closestStopName = train.closestStop; // Fall back to ID if name not found
    }

    auto nextStopIt = stopIdToNameMap.find(train.nextStop);
    if (nextStopIt != stopIdToNameMap.end()) {
      train.nextStopName = nextStopIt->second;
    } else {
      ESP_LOGW(TAG, "Stop name not found for nextStop ID: %s", train.nextStop.c_str());
      train.nextStopName = train.nextStop; // Fall back to ID if name not found
    }

    // Merge trip information if available
    auto tripIt = tripMap.find(train.tripId);
    if (tripIt != tripMap.end()) {
      TripInfo& tripInfo = tripIt->second;
      train.directionId = tripInfo.directionId;
      train.routeId = tripInfo.routeId;
      train.tripHeadsign = tripInfo.tripHeadsign;
    }

    // Set the line identifier
    train.line = line;

    // Add to the list
    trainDataList.push_back(train);

    // Log parsed data
    ESP_LOGI(TAG, "Train: tripId=%s, closestStop=%s (%s), closestStopOffset=%d, nextStop=%s (%s), nextStopOffset=%d, direction=%s, route=%s, headsign=%s, line=%s",
      train.tripId.c_str(),
      train.closestStop.c_str(),
      train.closestStopName.c_str(),
      train.closestStopTimeOffset,
      train.nextStop.c_str(),
      train.nextStopName.c_str(),
      train.nextStopTimeOffset,
      train.directionId.c_str(),
      train.routeId.c_str(),
      train.tripHeadsign.c_str(),
      train.line.c_str());
  }

  ESP_LOGI(TAG, "Processed %d train positions", trainDataList.size());
  return true;
}

void TrainDataManager::fetchTrainDataForRoute(const String& routeId, const String& lineName, const String& apiKey) {
  String url = String(API_BASE_URL) + "/trips-for-route/" + routeId + ".json?" + API_KEY_PARAM + "=" + apiKey;
  ESP_LOGI(TAG, "Fetching data for %s (route: %s)", lineName.c_str(), routeId.c_str());
  
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    WiFiClient* stream = http.getStreamPtr();

    if (stream == nullptr) {
      ESP_LOGE(TAG, "Failed to get HTTP stream for %s. URL: %s", lineName.c_str(), url.c_str());
    } else {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, *stream);

      if (error) {
        ESP_LOGE(TAG, "JSON parsing failed for %s: %s. URL: %s", lineName.c_str(), error.c_str(), url.c_str());
      } else {
        ESP_LOGI(TAG, "Successfully retrieved %s train data", lineName.c_str());
        parseTrainDataFromJson(doc, lineName);
      }
    }
  } else {
    ESP_LOGW(TAG, "HTTP request failed for %s with code %d. URL: %s. Will retry on next update cycle.", 
             lineName.c_str(), httpCode, url.c_str());
  }

  http.end();
}

void TrainDataManager::updateTrainPositions() {
  static const char* SAMPLE_DATA_PATH = "/data.json";

  String apiKey = preferencesManager.getApiKey();

  // Clear previous train data at the start of each update
  trainDataList.clear();

  if (apiKey.isEmpty()) {
    ESP_LOGW(TAG, "API key not configured, loading sample data from %s", SAMPLE_DATA_PATH);
    ESP_LOGI(TAG, "Note: Sample data only contains Line 1 trains");

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

    if (parseTrainDataFromJson(doc, LINE_1_NAME)) {
      ESP_LOGI(TAG, "Successfully loaded sample train data from LittleFS");
    }

    return;
  }

  ESP_LOGI(TAG, "Updating train positions...");

  // Fetch data for both lines
  fetchTrainDataForRoute(LINE_1_ROUTE_ID, LINE_1_NAME, apiKey);
  fetchTrainDataForRoute(LINE_2_ROUTE_ID, LINE_2_NAME, apiKey);
  
  ESP_LOGI(TAG, "Total trains from both lines: %d", trainDataList.size());
}
