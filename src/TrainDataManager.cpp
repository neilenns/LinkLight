#include "TrainDataManager.h"
#include <HTTPClient.h>
#include <LittleFS.h>
#include "LogManager.h"
#include <map>
#include "config.h"
#include "PreferencesManager.h"
#include "PSRAMJsonAllocator.h"
#include "PSRAMString.h"

static const char* LOG_TAG = "TrainDataManager";
static const float MIN_SCHEDULED_DISTANCE_THRESHOLD = 0.001f;

TrainDataManager trainDataManager;

// PSRAM-backed map types for temporary data structures
// Using PSRAM allocator for both the map nodes and the String-to-PSRAMString conversions
template<typename Key, typename Value>
using PSRAMMap = std::map<Key, Value, std::less<Key>, esp32_psram::AllocatorPSRAM<std::pair<const Key, Value>>>;

// Trip information structure
struct TripInfo {
  TrainDirection directionId;
  String routeId;
  String tripHeadsign;
};

bool TrainDataManager::parseTrainDataFromJson(JsonDocument& doc, const String& line) {
  // Get the data object
  JsonObject data = doc["data"];
  if (data.isNull()) {
    LINK_LOGW(LOG_TAG, "JSON response missing 'data' object");
    return false;
  }

  // First, build a map of trip information using PSRAM allocator
  PSRAMMap<String, TripInfo> tripMap;
  JsonArray trips = data["references"]["trips"];
  if (!trips.isNull()) {
    for (JsonObject trip : trips) {
      String tripId = trip["id"].as<String>();
      TripInfo info;
      String directionStr = trip["directionId"].as<String>();
      info.directionId = (directionStr == "1") ? TrainDirection::NORTHBOUND : TrainDirection::SOUTHBOUND;
      info.routeId = trip["routeId"].as<String>();
      info.tripHeadsign = trip["tripHeadsign"].as<String>();
      tripMap[tripId] = info;
    }
    LINK_LOGI(LOG_TAG, "Loaded %d trip references", tripMap.size());
  }

  // Build a map of stop IDs to stop names using PSRAM allocator
  PSRAMMap<String, String> stopIdToNameMap;
  JsonArray stops = data["references"]["stops"];
  if (!stops.isNull()) {
    for (JsonObject stop : stops) {
      String stopId = stop["id"].as<String>();
      String stopName = stop["name"].as<String>();
      stopIdToNameMap[stopId] = stopName;
    }
    LINK_LOGI(LOG_TAG, "Loaded %d stop references", stopIdToNameMap.size());
  }

  // Process the list array
  JsonArray list = data["list"];
  if (list.isNull()) {
    LINK_LOGW(LOG_TAG, "JSON response missing 'data.list' array");
    return false;
  }

  // Reserve memory to avoid reallocations during push_back operations
  trainDataList.reserve(list.size());

  for (JsonObject item : list) {
    TrainData train;

    // Extract tripId from the list item
    train.tripId = item["tripId"].as<String>();

    // Validate critical fields - skip trains with missing data
    // Note: We skip trains missing critical position/timing data (status, nextStop, nextStopTimeOffset)
    // but allow trains that haven't started yet (scheduledDistanceAlongTrip == 0)
    JsonObject status = item["status"];
    if (status.isNull()) {
      LINK_LOGW(LOG_TAG, "Status missing for trip %s", train.tripId.c_str());
      continue;
    }

    // Check for nextStop
    if (status["nextStop"].isNull()) {
      LINK_LOGW(LOG_TAG, "No next stop for trip %s", train.tripId.c_str());
      continue;
    }
    train.nextStop = status["nextStop"].as<String>();

    // Check for nextStopTimeOffset
    if (status["nextStopTimeOffset"].isNull()) {
      LINK_LOGW(LOG_TAG, "No next stop time offset for trip %s", train.tripId.c_str());
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
        LINK_LOGW(LOG_TAG, "Trip %s not in progress yet, scheduledDistanceAlongTrip: %.2f", 
                 train.tripId.c_str(), scheduledDistance);
      }
    } else {
      LINK_LOGW(LOG_TAG, "Trip %s not in progress yet, no scheduledDistanceAlongTrip", 
               train.tripId.c_str());
    }

    // Look up stop names from the stops map
    auto closestStopIt = stopIdToNameMap.find(train.closestStop);
    if (closestStopIt != stopIdToNameMap.end()) {
      train.closestStopName = closestStopIt->second;
    } else {
      LINK_LOGW(LOG_TAG, "Stop name not found for closestStop ID: %s", train.closestStop.c_str());
      train.closestStopName = train.closestStop; // Fall back to ID if name not found
    }

    auto nextStopIt = stopIdToNameMap.find(train.nextStop);
    if (nextStopIt != stopIdToNameMap.end()) {
      train.nextStopName = nextStopIt->second;
    } else {
      LINK_LOGW(LOG_TAG, "Stop name not found for nextStop ID: %s", train.nextStop.c_str());
      train.nextStopName = train.nextStop; // Fall back to ID if name not found
    }

    // Merge trip information if available
    auto tripIt = tripMap.find(train.tripId);
    if (tripIt != tripMap.end()) {
      TripInfo& tripInfo = tripIt->second;
      train.direction = tripInfo.directionId;
      train.routeId = tripInfo.routeId;
      train.tripHeadsign = tripInfo.tripHeadsign;
    }

    // Set the line identifier
    train.line = line;

    // Determine train state: AT_STATION or MOVING
    // If closestStopTimeOffset is negative (train has left the station) and significant,
    // the train is moving between stations. Otherwise, it's at a station.
    if (train.closestStopTimeOffset < -MIN_DEPARTED_SECONDS) {
      train.state = TrainState::MOVING;
    } else {
      train.state = TrainState::AT_STATION;
    }

    // Add to the list
    trainDataList.push_back(train);

    // Log parsed data
    LINK_LOGD(LOG_TAG, "Train: tripId=%s, closestStop=%s (%s), closestStopOffset=%d, nextStop=%s (%s), nextStopOffset=%d, direction=%s, route=%s, headsign=%s, line=%s, state=%s",
      train.tripId.c_str(),
      train.closestStop.c_str(),
      train.closestStopName.c_str(),
      train.closestStopTimeOffset,
      train.nextStop.c_str(),
      train.nextStopName.c_str(),
      train.nextStopTimeOffset,
      train.direction == TrainDirection::NORTHBOUND ? "Northbound" : "Southbound",
      train.routeId.c_str(),
      train.tripHeadsign.c_str(),
      train.line.c_str(),
      train.state == TrainState::AT_STATION ? "AT_STATION" : "MOVING");
  }

  LINK_LOGI(LOG_TAG, "Processed %d train positions", trainDataList.size());
  return true;
}

void TrainDataManager::fetchTrainDataForRoute(const String& routeId, const String& lineName, const String& apiKey) {
  // Use static buffer to avoid heap allocation for URL string on every call
  char url[256];
  snprintf(url, sizeof(url), "%s/trips-for-route/%s.json?%s=%s", 
           API_BASE_URL, routeId.c_str(), API_KEY_PARAM, apiKey.c_str());
  
  LINK_LOGI(LOG_TAG, "Fetching data for %s (route: %s)", lineName.c_str(), routeId.c_str());
  
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    WiFiClient* stream = http.getStreamPtr();

    if (stream == nullptr) {
      LINK_LOGE(LOG_TAG, "Failed to get HTTP stream for %s. URL: %s", lineName.c_str(), url);
    } else {
      JsonDocument doc(PSRAMJsonAllocator::instance());
      DeserializationError error = deserializeJson(doc, *stream);

      if (error) {
        LINK_LOGE(LOG_TAG, "JSON parsing failed for %s: %s. URL: %s", lineName.c_str(), error.c_str(), url);
      } else {
        LINK_LOGI(LOG_TAG, "Successfully retrieved %s train data", lineName.c_str());
        parseTrainDataFromJson(doc, lineName);
      }
    }
  } else {
    LINK_LOGW(LOG_TAG, "HTTP request failed for %s with code %d. URL: %s. Will retry on next update cycle.", 
             lineName.c_str(), httpCode, url);
  }

  http.end();
}

void TrainDataManager::updateTrainPositions() {
  static const char* SAMPLE_DATA_PATH = "/data.json";

  String apiKey = preferencesManager.getApiKey();

  // Clear previous train data at the start of each update
  trainDataList.clear();

  if (apiKey.isEmpty()) {
    LINK_LOGW(LOG_TAG, "API key not configured, loading sample data from %s", SAMPLE_DATA_PATH);
    LINK_LOGI(LOG_TAG, "Note: Sample data only contains Line 1 trains");

    File sampleFile = LittleFS.open(SAMPLE_DATA_PATH, "r");
    if (!sampleFile) {
      LINK_LOGE(LOG_TAG, "Sample data not found.");
      return;
    }

    JsonDocument doc(PSRAMJsonAllocator::instance());
    DeserializationError error = deserializeJson(doc, sampleFile);
    sampleFile.close();
    if (error) {
      LINK_LOGE(LOG_TAG, "Sample JSON parsing failed: %s", error.c_str());
      return;
    }

    if (parseTrainDataFromJson(doc, LINE_1_NAME)) {
      LINK_LOGI(LOG_TAG, "Successfully loaded sample train data from LittleFS");
    }

    return;
  }

  LINK_LOGI(LOG_TAG, "Updating train positions...");

  // Fetch data for both lines
  fetchTrainDataForRoute(LINE_1_ROUTE_ID, LINE_1_NAME, apiKey);
  fetchTrainDataForRoute(LINE_2_ROUTE_ID, LINE_2_NAME, apiKey);
  
  LINK_LOGI(LOG_TAG, "Total trains from both lines: %d", trainDataList.size());
}
