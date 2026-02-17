#include "TrainDataManager.h"
#include <HTTPClient.h>
#include <LittleFS.h>
#include "LogManager.h"
#include <map>
#include "config.h"
#include "PreferencesManager.h"
#include <esp_task_wdt.h>

static const char* TAG = "TrainDataManager";
static const float MIN_SCHEDULED_DISTANCE_THRESHOLD = 0.001f;

TrainDataManager trainDataManager;

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
    LINK_LOGW(TAG, "JSON response missing 'data' object");
    return false;
  }

  // First, build a map of trip information
  std::map<String, TripInfo> tripMap;
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
    LINK_LOGI(TAG, "Loaded %d trip references", tripMap.size());
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
    LINK_LOGI(TAG, "Loaded %d stop references", stopIdToNameMap.size());
  }

  // Process the list array
  JsonArray list = data["list"];
  if (list.isNull()) {
    LINK_LOGW(TAG, "JSON response missing 'data.list' array");
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
      LINK_LOGW(TAG, "Status missing for trip %s", train.tripId.c_str());
      continue;
    }

    // Check for nextStop
    if (status["nextStop"].isNull()) {
      LINK_LOGW(TAG, "No next stop for trip %s", train.tripId.c_str());
      continue;
    }
    train.nextStop = status["nextStop"].as<String>();

    // Check for nextStopTimeOffset
    if (status["nextStopTimeOffset"].isNull()) {
      LINK_LOGW(TAG, "No next stop time offset for trip %s", train.tripId.c_str());
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
        LINK_LOGW(TAG, "Trip %s not in progress yet, scheduledDistanceAlongTrip: %.2f", 
                 train.tripId.c_str(), scheduledDistance);
      }
    } else {
      LINK_LOGW(TAG, "Trip %s not in progress yet, no scheduledDistanceAlongTrip", 
               train.tripId.c_str());
    }

    // Look up stop names from the stops map
    auto closestStopIt = stopIdToNameMap.find(train.closestStop);
    if (closestStopIt != stopIdToNameMap.end()) {
      train.closestStopName = closestStopIt->second;
    } else {
      LINK_LOGW(TAG, "Stop name not found for closestStop ID: %s", train.closestStop.c_str());
      train.closestStopName = train.closestStop; // Fall back to ID if name not found
    }

    auto nextStopIt = stopIdToNameMap.find(train.nextStop);
    if (nextStopIt != stopIdToNameMap.end()) {
      train.nextStopName = nextStopIt->second;
    } else {
      LINK_LOGW(TAG, "Stop name not found for nextStop ID: %s", train.nextStop.c_str());
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
    LINK_LOGD(TAG, "Train: tripId=%s, closestStop=%s (%s), closestStopOffset=%d, nextStop=%s (%s), nextStopOffset=%d, direction=%s, route=%s, headsign=%s, line=%s, state=%s",
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

  LINK_LOGI(TAG, "Processed %d train positions", trainDataList.size());
  return true;
}

void TrainDataManager::fetchTrainDataForRoute(const String& routeId, const String& lineName, const String& apiKey) {
  // Log free heap before operation
  size_t freeHeapBefore = ESP.getFreeHeap();
  LINK_LOGD(TAG, "Free heap before API call: %d bytes", freeHeapBefore);
  
  String url = String(API_BASE_URL) + "/trips-for-route/" + routeId + ".json?" + API_KEY_PARAM + "=" + apiKey;
  LINK_LOGI(TAG, "Fetching data for %s (route: %s)", lineName.c_str(), routeId.c_str());
  
  // Feed watchdog before starting potentially long operation
  esp_task_wdt_reset();
  
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(url);
  
  // Feed watchdog before GET request
  esp_task_wdt_reset();
  int httpCode = http.GET();
  
  // Feed watchdog after GET completes
  esp_task_wdt_reset();

  if (httpCode == HTTP_CODE_OK) {
    WiFiClient* stream = http.getStreamPtr();

    if (stream == nullptr) {
      LINK_LOGE(TAG, "Failed to get HTTP stream for %s. URL: %s", lineName.c_str(), url.c_str());
    } else {
      // ArduinoJson 7 uses JsonDocument with heap allocator by default
      // Allocate a buffer for deserialization to prevent heap fragmentation
      JsonDocument doc;
      
      // Feed watchdog before JSON parsing
      esp_task_wdt_reset();
      DeserializationError error = deserializeJson(doc, *stream, DeserializationOption::NestingLimit(20));
      
      // Feed watchdog after JSON parsing
      esp_task_wdt_reset();

      if (error) {
        LINK_LOGE(TAG, "JSON parsing failed for %s: %s. URL: %s", lineName.c_str(), error.c_str(), url.c_str());
        if (error == DeserializationError::NoMemory) {
          LINK_LOGE(TAG, "Not enough memory to parse JSON response. Free heap: %d bytes", ESP.getFreeHeap());
        }
      } else {
        LINK_LOGI(TAG, "Successfully retrieved %s train data (JSON used %u bytes)", lineName.c_str(), doc.memoryUsage());
        parseTrainDataFromJson(doc, lineName);
      }
      
      // Clear the document to free memory immediately
      doc.clear();
      doc.shrinkToFit();
    }
  } else {
    LINK_LOGW(TAG, "HTTP request failed for %s with code %d. URL: %s. Will retry on next update cycle.", 
             lineName.c_str(), httpCode, url.c_str());
  }

  http.end();
  
  // Log free heap after operation
  size_t freeHeapAfter = ESP.getFreeHeap();
  LINK_LOGD(TAG, "Free heap after API call: %d bytes (delta: %d bytes)", 
           freeHeapAfter, (int)freeHeapAfter - (int)freeHeapBefore);
}

void TrainDataManager::updateTrainPositions() {
  static const char* SAMPLE_DATA_PATH = "/data.json";

  // Log free heap at the start of update cycle
  size_t freeHeapStart = ESP.getFreeHeap();
  LINK_LOGI(TAG, "=== Starting train position update (Free heap: %d bytes) ===", freeHeapStart);
  
  // Feed watchdog at start of update
  esp_task_wdt_reset();

  String apiKey = preferencesManager.getApiKey();

  // Clear previous train data at the start of each update
  trainDataList.clear();
  
  // Shrink vector to free unused capacity
  trainDataList.shrink_to_fit();
  
  // Reserve space for typical number of trains (reduces reallocations)
  trainDataList.reserve(20);  // Typical API response has 10-15 trains

  if (apiKey.isEmpty()) {
    LINK_LOGW(TAG, "API key not configured, loading sample data from %s", SAMPLE_DATA_PATH);
    LINK_LOGI(TAG, "Note: Sample data only contains Line 1 trains");

    File sampleFile = LittleFS.open(SAMPLE_DATA_PATH, "r");
    if (!sampleFile) {
      LINK_LOGE(TAG, "Sample data not found.");
      return;
    }

    // ArduinoJson 7 uses JsonDocument with heap allocator by default
    JsonDocument doc;
    
    // Feed watchdog before JSON parsing
    esp_task_wdt_reset();
    DeserializationError error = deserializeJson(doc, sampleFile, DeserializationOption::NestingLimit(20));
    sampleFile.close();
    
    // Feed watchdog after JSON parsing
    esp_task_wdt_reset();
    
    if (error) {
      LINK_LOGE(TAG, "Sample JSON parsing failed: %s", error.c_str());
      if (error == DeserializationError::NoMemory) {
        LINK_LOGE(TAG, "Not enough memory to parse sample JSON. Free heap: %d bytes", ESP.getFreeHeap());
      }
      return;
    }

    if (parseTrainDataFromJson(doc, LINE_1_NAME)) {
      LINK_LOGI(TAG, "Successfully loaded sample train data from LittleFS (JSON used %u bytes)", doc.memoryUsage());
    }
    
    // Clear the document to free memory immediately
    doc.clear();
    doc.shrinkToFit();

    return;
  }

  LINK_LOGI(TAG, "Updating train positions...");

  // Fetch data for both lines
  fetchTrainDataForRoute(LINE_1_ROUTE_ID, LINE_1_NAME, apiKey);
  
  // Feed watchdog between route fetches
  esp_task_wdt_reset();
  
  // fetchTrainDataForRoute(LINE_2_ROUTE_ID, LINE_2_NAME, apiKey);
  
  LINK_LOGI(TAG, "Total trains from both lines: %d", trainDataList.size());
  
  // Log free heap at the end of update cycle
  size_t freeHeapEnd = ESP.getFreeHeap();
  int heapDelta = (int)freeHeapEnd - (int)freeHeapStart;
  LINK_LOGI(TAG, "=== Completed train position update (Free heap: %d bytes, delta: %d bytes) ===", 
           freeHeapEnd, heapDelta);
  
  // Warn if heap is getting low
  if (freeHeapEnd < 50000) {
    LINK_LOGW(TAG, "WARNING: Low free heap detected: %d bytes", freeHeapEnd);
  }
  
  // Feed watchdog at end of update
  esp_task_wdt_reset();
}
