#include "WebServerManager.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Ministache.h>
#include "LogManager.h"
#include "FileSystemManager.h"
#include "PreferencesManager.h"
#include "PSRAMJsonAllocator.h"
#include "TrainDataManager.h"
#include "LEDController.h"

static const char* LOG_TAG = "WebServerManager";

WebServerManager webServerManager;

void WebServerManager::setup() {
  LINK_LOGD(LOG_TAG, "Setting up web server...");
  
  // Register handlers
  server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server.on("/config", HTTP_GET, [this]() { this->handleConfig(); });
  server.on("/config", HTTP_POST, [this]() { this->handleSaveConfig(); });
  server.on("/test-station", HTTP_POST, [this]() { this->handleTestStation(); });
  server.on("/logs", HTTP_GET, [this]() { this->handleLogs(); });
  server.on("/api/logs", HTTP_GET, [this]() { this->handleLogsData(); });
  server.on("/trains", HTTP_GET, [this]() { this->handleTrains(); });
  
  // Start server
  server.begin();
  
  // Setup WebSocket
  webSocket.begin();
  webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    this->handleWebSocketEvent(num, type, payload, length);
  });
  
  LINK_LOGI(LOG_TAG, "WebSocket server started on port %d", WEB_SOCKET_PORT);
}

void WebServerManager::handleClient() {
  server.handleClient();
  webSocket.loop();
}

void WebServerManager::handleRoot() {
  String html = fileSystemManager.readFile("/index.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load index.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  // Create data for Ministache template
  JsonDocument data(PSRAMJsonAllocator::instance());
  data["ipAddress"] = WiFi.localIP().toString();
  data["hostname"] = preferencesManager.getHostname();
  
  String output = ministache::render(html, data);
  server.send(200, "text/html", output);
}

void WebServerManager::handleConfig() {
  String html = fileSystemManager.readFile("/config.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load config.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  // Create data for Ministache template
  JsonDocument data(PSRAMJsonAllocator::instance());
  data["apiKey"] = preferencesManager.getApiKey();
  data["hostname"] = preferencesManager.getHostname();
  data["timezone"] = preferencesManager.getTimezone();
  data["focusedVehicleId"] = preferencesManager.getFocusedVehicleId();
  data["updateInterval"] = preferencesManager.getUpdateInterval();
  data["line1Color"] = preferencesManager.getLine1Color();
  data["line2Color"] = preferencesManager.getLine2Color();
  data["sharedColor"] = preferencesManager.getSharedColor();
  
  // Add train data for the dropdown
  JsonArray trainsArray = data["trains"].to<JsonArray>();
  const esp32_psram::VectorPSRAM<TrainData>& trains = trainDataManager.getTrainDataList();
  
  for (const TrainData& train : trains) {
    JsonObject trainObj = trainsArray.add<JsonObject>();
    trainObj["vehicleId"] = train.vehicleId;
    trainObj["station"] = train.state == TrainState::AT_STATION ? train.closestStopName : train.nextStopName;
    trainObj["line"] = String("Line ") + String(static_cast<int>(train.line));
    trainObj["direction"] = train.direction == TrainDirection::NORTHBOUND ? "Northbound" : "Southbound";
  }
  
  // Add station data for the test dropdown
  JsonArray stationsArray = data["stations"].to<JsonArray>();
  const std::map<String, StationLEDMapping>& stationMap = ledController.getStationMap();
  
  for (const auto& station : stationMap) {
    JsonObject stationObj = stationsArray.add<JsonObject>();
    stationObj["name"] = station.first;
  }
  
  String output = ministache::render(html, data);
  server.send(200, "text/html", output);
}

void WebServerManager::handleSaveConfig() {
  // Validate and sanitize inputs
  if (server.hasArg("apiKey")) {
    String apiKey = server.arg("apiKey");
    // Limit length to prevent excessive storage use
    if (apiKey.length() > MAX_PREFERENCE_LENGTH) {
      apiKey = apiKey.substring(0, MAX_PREFERENCE_LENGTH);
    }
    preferencesManager.setApiKey(apiKey);
  }
  if (server.hasArg("hostname")) {
    String hostname = server.arg("hostname");
    // Limit length to prevent excessive storage use
    if (hostname.length() > MAX_PREFERENCE_LENGTH) {
      hostname = hostname.substring(0, MAX_PREFERENCE_LENGTH);
    }
    // Use default if empty
    if (hostname.isEmpty()) {
      hostname = DEFAULT_HOSTNAME;
    } else {
      // Validate RFC 1123 compliance: only alphanumeric and hyphens, no leading/trailing hyphens
      String validHostname = "";
      for (size_t i = 0; i < hostname.length(); i++) {
        char c = hostname.charAt(i);
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
            (c >= '0' && c <= '9') || (c == '-' && i > 0 && i < hostname.length() - 1)) {
          validHostname += c;
        }
      }
      // Use the validated hostname or default if nothing valid
      hostname = validHostname.isEmpty() ? DEFAULT_HOSTNAME : validHostname;
    }
    preferencesManager.setHostname(hostname);
  }
  if (server.hasArg("timezone")) {
    String timezone = server.arg("timezone");
    // Limit length to prevent excessive storage use
    if (timezone.length() > MAX_PREFERENCE_LENGTH) {
      timezone = timezone.substring(0, MAX_PREFERENCE_LENGTH);
    }
    // Use default if empty
    if (timezone.isEmpty()) {
      timezone = DEFAULT_TIMEZONE;
    }
    preferencesManager.setTimezone(timezone);
  }
  
  // Handle focused train ID (not persisted)
  if (server.hasArg("focusedVehicleId")) {
    String focusedVehicleId = server.arg("focusedVehicleId");
    preferencesManager.setFocusedVehicleId(focusedVehicleId);
  }
  
  // Handle update interval with validation (15-60 seconds)
  if (server.hasArg("updateInterval")) {
    int interval = server.arg("updateInterval").toInt();
    // Validate range: 15-60 seconds
    if (interval >= 15 && interval <= 60) {
      preferencesManager.setUpdateInterval(interval);
    } else {
      // Use default if out of range
      preferencesManager.setUpdateInterval(DEFAULT_UPDATE_INTERVAL);
    }
  }
  
  // Handle Line 1 color
  if (server.hasArg("line1Color")) {
    String line1Color = server.arg("line1Color");
    // Validate hex color format
    if (line1Color.startsWith("#") && line1Color.length() == 7) {
      preferencesManager.setLine1Color(line1Color);
    }
  }
  
  // Handle Line 2 color
  if (server.hasArg("line2Color")) {
    String line2Color = server.arg("line2Color");
    // Validate hex color format
    if (line2Color.startsWith("#") && line2Color.length() == 7) {
      preferencesManager.setLine2Color(line2Color);
    }
  }
  
  // Handle shared/overlap color
  if (server.hasArg("sharedColor")) {
    String sharedColor = server.arg("sharedColor");
    // Validate hex color format
    if (sharedColor.startsWith("#") && sharedColor.length() == 7) {
      preferencesManager.setSharedColor(sharedColor);
    }
  }
  
  preferencesManager.save();
  
  String html = fileSystemManager.readFile("/config_saved.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load config_saved.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  server.send(200, "text/html", html);
}

void WebServerManager::handleTestStation() {
  // Get the station name from the request
  if (!server.hasArg("stationName")) {
    server.send(400, "text/plain", "Missing stationName parameter");
    return;
  }
  
  String stationName = server.arg("stationName");
  
  // Call the LED controller to test the station
  ledController.testStationLEDs(stationName);
  
  // Send success response
  server.send(200, "text/plain", "OK");
}

void WebServerManager::handleLogs() {
  String html = fileSystemManager.readFile("/logs.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load logs.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  server.send(200, "text/html", html);
}

void WebServerManager::handleLogsData() {
  // Get logs from LogManager
  esp32_psram::VectorPSRAM<LogEntry> logs = logManager.getLogs();
  
  // Create JSON response
  JsonDocument doc(PSRAMJsonAllocator::instance());
  JsonArray logsArray = doc["logs"].to<JsonArray>();
  
  for (const auto& entry : logs) {
    JsonObject logObj = logsArray.add<JsonObject>();
    logObj["timestamp"] = entry.timestamp;
    logObj["level"] = entry.level;
    logObj["tag"] = entry.tag;
    logObj["message"] = entry.message;
  }
  
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  
  server.send(200, "application/json", jsonResponse);
}

void WebServerManager::handleTrains() {
  String html = fileSystemManager.readFile("/trains.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load trains.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  server.send(200, "text/html", html);
}

void WebServerManager::sendTrainData(uint8_t clientNum) {
  JsonDocument doc(PSRAMJsonAllocator::instance());
  doc["type"] = "trains";
  JsonArray trainsArray = doc["trains"].to<JsonArray>();
  
  const esp32_psram::VectorPSRAM<TrainData>& trains = trainDataManager.getTrainDataList();
  for (const TrainData& train : trains) {
    JsonObject trainObj = trainsArray.add<JsonObject>();
    trainObj["vehicleId"] = train.vehicleId;
    trainObj["line"] = static_cast<int>(train.line);
    trainObj["direction"] = train.direction == TrainDirection::NORTHBOUND ? "Northbound" : "Southbound";
    trainObj["headsign"] = train.tripHeadsign;
    trainObj["state"] = train.state == TrainState::AT_STATION ? "At Station" : "Moving";
    trainObj["closestStop"] = train.closestStopName;
    trainObj["nextStop"] = train.nextStopName;
    trainObj["nextStopOffset"] = train.nextStopTimeOffset;
  }
  
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  webSocket.sendTXT(clientNum, jsonResponse);
}

void WebServerManager::broadcastTrainData() {
  if (webSocket.connectedClients() == 0) {
    return;
  }
  
  JsonDocument doc(PSRAMJsonAllocator::instance());
  doc["type"] = "trains";
  JsonArray trainsArray = doc["trains"].to<JsonArray>();
  
  const esp32_psram::VectorPSRAM<TrainData>& trains = trainDataManager.getTrainDataList();
  for (const TrainData& train : trains) {
    JsonObject trainObj = trainsArray.add<JsonObject>();
    trainObj["vehicleId"] = train.vehicleId;
    trainObj["line"] = static_cast<int>(train.line);
    trainObj["direction"] = train.direction == TrainDirection::NORTHBOUND ? "Northbound" : "Southbound";
    trainObj["headsign"] = train.tripHeadsign;
    trainObj["state"] = train.state == TrainState::AT_STATION ? "At Station" : "Moving";
    trainObj["closestStop"] = train.closestStopName;
    trainObj["nextStop"] = train.nextStopName;
    trainObj["nextStopOffset"] = train.nextStopTimeOffset;
  }
  
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  webSocket.broadcastTXT(jsonResponse);
}

void WebServerManager::handleWebSocketEvent(uint8_t clientNum, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      LINK_LOGD(LOG_TAG, "WebSocket client #%u disconnected", clientNum);
      break;
      
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(clientNum);
      LINK_LOGI(LOG_TAG, "WebSocket client #%u connected from %s", clientNum, ip.toString().c_str());
      
      // Send initial logs to the newly connected client
      esp32_psram::VectorPSRAM<LogEntry> logs = logManager.getLogs();
      
      // Create JSON for initial logs
      JsonDocument doc(PSRAMJsonAllocator::instance());
      doc["type"] = "initial";
      JsonArray logsArray = doc["logs"].to<JsonArray>();
      
      for (const auto& entry : logs) {
        JsonObject logObj = logsArray.add<JsonObject>();
        logObj["timestamp"] = entry.timestamp;
        logObj["level"] = entry.level;
        logObj["tag"] = entry.tag;
        logObj["message"] = entry.message;
      }
      
      String jsonResponse;
      serializeJson(doc, jsonResponse);
      webSocket.sendTXT(clientNum, jsonResponse);
      
      // Send current train data to the newly connected client
      sendTrainData(clientNum);
      
      // Send current LED state to the newly connected client
      sendLEDState(clientNum);
      break;
    }
      
    case WStype_TEXT:
      // Handle incoming text messages if needed
      LINK_LOGD(LOG_TAG, "WebSocket message from client #%u: %s", clientNum, payload);
      break;
      
    case WStype_ERROR:
      LINK_LOGE(LOG_TAG, "WebSocket error on client #%u", clientNum);
      break;
      
    default:
      break;
  }
}

void WebServerManager::broadcastLog(const char* level, const char* tag, const char* message, unsigned long timestamp) {
  // Only broadcast if there are connected clients
  if (webSocket.connectedClients() == 0) {
    return;
  }
  
  // Create JSON for the new log entry
  JsonDocument doc(PSRAMJsonAllocator::instance());
  doc["type"] = "log";
  doc["timestamp"] = timestamp;
  doc["level"] = level;
  doc["tag"] = tag;
  doc["message"] = message;
  
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  
  // Broadcast to all connected clients
  webSocket.broadcastTXT(jsonResponse);
}

void WebServerManager::sendLEDState(uint8_t clientNum) {
  String jsonResponse;
  ledController.serializeLEDState(jsonResponse);
  webSocket.sendTXT(clientNum, jsonResponse);
}

void WebServerManager::broadcastLEDState() {
  if (webSocket.connectedClients() == 0) {
    return;
  }

  String jsonResponse;
  ledController.serializeLEDState(jsonResponse);
  webSocket.broadcastTXT(jsonResponse);
}
