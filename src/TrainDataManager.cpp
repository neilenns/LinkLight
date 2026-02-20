#include "TrainDataManager.h"
#include <HTTPClient.h>
#include <LittleFS.h>
#include "LogManager.h"
#include <map>
#include "config.h"
#include "PreferencesManager.h"
#include "PSRAMJsonAllocator.h"
#include "PSRAMString.h"
#include "StopData.h"

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

bool TrainDataManager::parseTrainDataFromJson(JsonDocument& doc, Line line) {
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
  }

  // Use hardcoded stop data instead of parsing from JSON
  // Stop information is now defined in StopData.h and never changes
  LINK_LOGI(LOG_TAG, "Loaded %u trips for %d Line", tripMap.size(), static_cast<int>(line));

  // Process the list array
  JsonArray list = data["list"];
  if (list.isNull()) {
    LINK_LOGW(LOG_TAG, "JSON response missing 'data.list' array");
    return false;
  }

  // Reserve memory to avoid reallocations during push_back operations
  buildingList.reserve(list.size());

  for (JsonObject item : list) {
    TrainData train;

    // Extract tripId from the list item
    train.tripId = item["tripId"].as<String>();

    // Trains without a status aren't actually running, so we skip them entirely instead of trying to parse incomplete data.
    JsonObject status = item["status"];
    if (status.isNull()) {
      LINK_LOGW(LOG_TAG, "Status missing for trip %s, skipping train", train.tripId.c_str());
      continue;
    }

    // Extract vehicleId from status.vehicleId
    train.vehicleId = status["vehicleId"].as<String>();
    
    // Fall back to last part of tripId if vehicleId is empty
    if (train.vehicleId.isEmpty()) {
      int lastUnderscore = train.tripId.lastIndexOf('_');
      if (lastUnderscore != -1 && lastUnderscore < train.tripId.length() - 1) {
        train.vehicleId = train.tripId.substring(lastUnderscore + 1);
      } else {
        // If no underscore found, use the whole tripId as fallback
        train.vehicleId = train.tripId;
      }
    }

    // Check for nextStop
    if (status["nextStop"].isNull()) {
      LINK_LOGW(LOG_TAG, "No next stop for vehicle %s", train.vehicleId.c_str());
      continue;
    }
    train.nextStop = status["nextStop"].as<String>();

    // Check for nextStopTimeOffset
    if (status["nextStopTimeOffset"].isNull()) {
      LINK_LOGW(LOG_TAG, "No next stop time offset for vehicle %s", train.vehicleId.c_str());
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
        LINK_LOGW(LOG_TAG, "Vehicle %s not in progress yet, scheduledDistanceAlongTrip: %.2f", 
                 train.vehicleId.c_str(), scheduledDistance);
      }
    } else {
      LINK_LOGW(LOG_TAG, "Vehicle %s not in progress yet, no scheduledDistanceAlongTrip", 
               train.vehicleId.c_str());
    }

    // Look up stop names from the hardcoded stop data
    auto closestStopIt = STOP_ID_TO_NAME.find(train.closestStop);
    if (closestStopIt != STOP_ID_TO_NAME.end()) {
      train.closestStopName = closestStopIt->second;
    } else {
      LINK_LOGW(LOG_TAG, "Stop name not found for closestStop ID: %s", train.closestStop.c_str());
      train.closestStopName = train.closestStop; // Fall back to ID if name not found
    }

    auto nextStopIt = STOP_ID_TO_NAME.find(train.nextStop);
    if (nextStopIt != STOP_ID_TO_NAME.end()) {
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
    // This took quite a bit of work to get right. Simply checking to see if nextStopTimeOffset and closestStopTimeOffset are 0
    // doesn't work, since it misses trains that are just barely arriving at the station but leave during the data refresh window.
    // Checking for the next and closest stop IDs (or offsets) being the same also doesn't work, since that's true any time the
    // train is past the halfway point of the two stations. 
    if (train.nextStopTimeOffset < AT_STATION_THRESHOLD) {
      train.state = TrainState::AT_STATION;
    } else {
      train.state = TrainState::MOVING;
    }

    // Add to the list
    buildingList.push_back(train);

    // If a focused train is set, log only that train's data
    String focusedVehicleId = preferencesManager.getFocusedVehicleId();
    if (!focusedVehicleId.isEmpty() && train.vehicleId != focusedVehicleId) {
      continue;
    }

    // Log parsed data
    LINK_LOGD(LOG_TAG, "Train: vehicleId=%s, closestStop=%s (%s), closestStopOffset=%d, nextStop=%s (%s), nextStopOffset=%d, direction=%s, route=%s, headsign=%s, line=%d, state=%s",
      train.vehicleId.c_str(),
      train.closestStop.c_str(),
      train.closestStopName.c_str(),
      train.closestStopTimeOffset,
      train.nextStop.c_str(),
      train.nextStopName.c_str(),
      train.nextStopTimeOffset,
      train.direction == TrainDirection::NORTHBOUND ? "Northbound" : "Southbound",
      train.routeId.c_str(),
      train.tripHeadsign.c_str(),
      static_cast<int>(train.line),
      train.state == TrainState::AT_STATION ? "AT_STATION" : "MOVING");
  }

  return true;
}

void TrainDataManager::fetchTrainDataForRoute(const String& routeId, Line line, const String& apiKey) {
  // Use static buffer to avoid heap allocation for URL string on every call
  char url[256];
  snprintf(url, sizeof(url), "%s/trips-for-route/%s.json?includeSchedule=false&%s=%s", 
           API_BASE_URL, routeId.c_str(), API_KEY_PARAM, apiKey.c_str());
  
  LINK_LOGD(LOG_TAG, "Fetching data for %d Line (route: %s)", static_cast<int>(line), routeId.c_str());
  
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    WiFiClient* stream = http.getStreamPtr();

    if (stream == nullptr) {
      LINK_LOGE(LOG_TAG, "Failed to get HTTP stream for %d Line. URL: %s", static_cast<int>(line), url);
    } else {
      JsonDocument doc(PSRAMJsonAllocator::instance());
      DeserializationError error = deserializeJson(doc, *stream);

      if (error) {
        LINK_LOGE(LOG_TAG, "JSON parsing failed for %d Line: %s. URL: %s", static_cast<int>(line), error.c_str(), url);
      } else {
        LINK_LOGD(LOG_TAG, "Successfully retrieved %d Line train data", static_cast<int>(line));
        parseTrainDataFromJson(doc, line);
      }
    }
  } else {
    LINK_LOGW(LOG_TAG, "HTTP request failed for %d Line with code %d. URL: %s. Will retry on next update cycle.", 
             static_cast<int>(line), httpCode, url);
  }

  http.end();
}

void TrainDataManager::buildTrainJsonObject(JsonObject trainObj, const TrainData& train) const {
  trainObj["vehicleId"] = train.vehicleId;
  trainObj["line"] = static_cast<int>(train.line);
  trainObj["direction"] = train.direction == TrainDirection::NORTHBOUND ? "Northbound" : "Southbound";
  trainObj["headsign"] = train.tripHeadsign;
  trainObj["state"] = train.state == TrainState::AT_STATION ? "At Station" : "Moving";
  trainObj["closestStop"] = train.closestStopName;
  trainObj["closestStopTimeOffset"] = train.closestStopTimeOffset;
  trainObj["nextStop"] = train.nextStopName;
  trainObj["nextStopTimeOffset"] = train.nextStopTimeOffset;
}

void TrainDataManager::getTrainDataAsJson(String& output) const {
  JsonDocument doc(PSRAMJsonAllocator::instance());
  doc["type"] = "trains";
  JsonArray trainsArray = doc["trains"].to<JsonArray>();

  if (dataMutex) xSemaphoreTake(dataMutex, portMAX_DELAY);
  for (const TrainData& train : trainDataList) {
    JsonObject trainObj = trainsArray.add<JsonObject>();
    buildTrainJsonObject(trainObj, train);
  }
  if (dataMutex) xSemaphoreGive(dataMutex);

  serializeJson(doc, output);
}

void TrainDataManager::updateTrainPositions() {
  static const char* SAMPLE_DATA_PATH = "/data.json";

  String apiKey = preferencesManager.getApiKey();

  // Clear the building list at the start of each update
  buildingList.clear();

  if (apiKey.isEmpty()) {
    LINK_LOGW(LOG_TAG, "API key not configured, loading sample data from %s", SAMPLE_DATA_PATH);

    File sampleFile = LittleFS.open(SAMPLE_DATA_PATH, "r");
    if (!sampleFile) {
      LINK_LOGE(LOG_TAG, "Sample data not found.");
    } else {
      JsonDocument doc(PSRAMJsonAllocator::instance());
      DeserializationError error = deserializeJson(doc, sampleFile);
      sampleFile.close();
      if (error) {
        LINK_LOGE(LOG_TAG, "Sample JSON parsing failed: %s", error.c_str());
      } else if (parseTrainDataFromJson(doc, Line::LINE_1)) {
        LINK_LOGI(LOG_TAG, "Successfully loaded sample train data from LittleFS");
      }
    }
  } else {
    LINK_LOGD(LOG_TAG, "Updating train positions...");

    // Fetch data for both lines
    fetchTrainDataForRoute(LINE_1_ROUTE_ID, Line::LINE_1, apiKey);
    fetchTrainDataForRoute(LINE_2_ROUTE_ID, Line::LINE_2, apiKey);
  }

  // Swap the completed building list into the active list under mutex
  if (dataMutex) xSemaphoreTake(dataMutex, portMAX_DELAY);
  buildingList.swap(trainDataList);
  if (dataMutex) xSemaphoreGive(dataMutex);
}
