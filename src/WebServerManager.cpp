#include "WebServerManager.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Ministache.h>
#include <Update.h>
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
  
  // CSS files
  server.on("/global.css", HTTP_GET, [this]() { this->handleStaticFile("/global.css", "text/css"); });
  server.on("/notyf.min.css", HTTP_GET, [this]() { this->handleStaticFile("/notyf.min.css", "text/css"); });
  
  // Javascript files
  server.on("/notyf.min.js", HTTP_GET, [this]() { this->handleStaticFile("/notyf.min.js", "application/javascript"); });
  server.on("/notyf.js", HTTP_GET, [this]() { this->handleStaticFile("/notyf.js", "application/javascript"); });

  server.on("/config", HTTP_GET, [this]() { this->handleConfig(); });
  server.on("/config", HTTP_POST, [this]() { this->handleSaveConfig(); });
  server.on("/test-station", HTTP_POST, [this]() { this->handleTestStation(); });
  server.on("/logs", HTTP_GET, [this]() { this->handleStaticFile("/logs.html", "text/html"); });
  server.on("/api/logs", HTTP_GET, [this]() { this->handleLogsData(); });
  server.on("/trains", HTTP_GET, [this]() { this->handleStaticFile("/trains.html", "text/html"); });
  server.on("/update", HTTP_GET, [this]() { this->handleStaticFile("/update.html", "text/html"); });
  server.on("/update/firmware", HTTP_POST,
    [this]() { this->handleUpdateFirmware(); },
    [this]() { this->handleUpdateFirmwareUpload(); });
  server.on("/update/filesystem", HTTP_POST,
    [this]() { this->handleUpdateFilesystem(); },
    [this]() { this->handleUpdateFilesystemUpload(); });
  
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

void WebServerManager::handleStaticFile(const String& path, const String& contentType) {
  String content = fileSystemManager.readFile(path.c_str());
  if (content.isEmpty()) {
    server.send(500, "text/plain", "Failed to load " + path + " - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }

  server.send(200, contentType, content);
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
  data["updateInterval"] = preferencesManager.getUpdateInterval();
  data["atStationThreshold"] = preferencesManager.getAtStationThreshold();
  data["line1Color"] = preferencesManager.getLine1Color();
  data["line2Color"] = preferencesManager.getLine2Color();
  data["sharedColor"] = preferencesManager.getSharedColor();
  
  // Add station data for the test dropdown
  JsonArray stationsArray = data["stations"].to<JsonArray>();
  const std::map<String, StationLEDMapping>& stationMap = ledController.getStationMap();
  
  for (const auto& station : stationMap) {
    JsonObject stationObj = stationsArray.add<JsonObject>();
    stationObj["name"] = station.first;
    String stationId = station.first;
    stationId.replace(" ", "_");
    stationObj["id"] = stationId;
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
  
  // Handle at-station threshold with validation (0-60 seconds)
  if (server.hasArg("atStationThreshold")) {
    int threshold = server.arg("atStationThreshold").toInt();
    // Validate range: 0-60 seconds
    if (threshold >= 0 && threshold <= 60) {
      preferencesManager.setAtStationThreshold(threshold);
    } else {
      // Use default if out of range
      preferencesManager.setAtStationThreshold(DEFAULT_AT_STATION_THRESHOLD);
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
  
  server.send(200, "text/plain", "OK");
}

void WebServerManager::handleTestStation() {
  // Get the station name from the request
  if (!server.hasArg("stationName")) {
    server.send(400, "text/plain", "Missing stationName parameter");
    return;
  }
  
  String stationName = server.arg("stationName");
  stationName.replace("_", " ");  // Convert underscores back to spaces (values in sl-option cannot include spaces)
  
  // Call the LED controller to test the station
  ledController.testStationLEDs(stationName);
  
  // Send success response
  server.send(200, "text/plain", "OK");
}

void WebServerManager::handleLogsData() {
  String jsonResponse;
  logManager.getLogsAsJson(jsonResponse);
  server.send(200, "application/json", jsonResponse);
}

void WebServerManager::handleUpdateFirmware() {
  if (Update.hasError()) {
    String error = Update.errorString();
    LINK_LOGE(LOG_TAG, "Firmware update failed: %s", error.c_str());
    server.send(500, "text/plain", "Update failed: " + error);
  } else {
    LINK_LOGI(LOG_TAG, "Firmware update successful, restarting...");
    server.send(200, "text/plain", "OK");
    delay(500);
    ESP.restart();
  }
}

void WebServerManager::handleUpdateFirmwareUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    LINK_LOGI(LOG_TAG, "Firmware update start: %s", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      LINK_LOGE(LOG_TAG, "Firmware update begin failed: %s", Update.errorString());
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!Update.hasError()) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        LINK_LOGE(LOG_TAG, "Firmware update write failed: %s", Update.errorString());
      }
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!Update.hasError()) {
      if (!Update.end(true)) {
        LINK_LOGE(LOG_TAG, "Firmware update end failed: %s", Update.errorString());
      } else {
        LINK_LOGI(LOG_TAG, "Firmware update complete: %u bytes", upload.totalSize);
      }
    }
  }
}

void WebServerManager::handleUpdateFilesystem() {
  if (Update.hasError()) {
    String error = Update.errorString();
    LINK_LOGE(LOG_TAG, "Filesystem update failed: %s", error.c_str());
    server.send(500, "text/plain", "Update failed: " + error);
  } else {
    LINK_LOGI(LOG_TAG, "Filesystem update successful, restarting...");
    server.send(200, "text/plain", "OK");
    delay(500);
    ESP.restart();
  }
}

void WebServerManager::handleUpdateFilesystemUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    LINK_LOGI(LOG_TAG, "Filesystem update start: %s", upload.filename.c_str());
    // U_SPIFFS is the correct command for any filesystem partition update on ESP32,
    // regardless of whether the partition is formatted as SPIFFS or LittleFS.
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
      LINK_LOGE(LOG_TAG, "Filesystem update begin failed: %s", Update.errorString());
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!Update.hasError()) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        LINK_LOGE(LOG_TAG, "Filesystem update write failed: %s", Update.errorString());
      }
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!Update.hasError()) {
      if (!Update.end(true)) {
        LINK_LOGE(LOG_TAG, "Filesystem update end failed: %s", Update.errorString());
      } else {
        LINK_LOGI(LOG_TAG, "Filesystem update complete: %u bytes", upload.totalSize);
      }
    }
  }
}

void WebServerManager::sendTrainData(int clientNum) {
  String jsonResponse;
  trainDataManager.getTrainDataAsJson(jsonResponse);
  if (clientNum == -1) {
    if (webSocket.connectedClients() == 0) {
      return;
    }
    webSocket.broadcastTXT(jsonResponse);
  } else {
    webSocket.sendTXT(static_cast<uint8_t>(clientNum), jsonResponse);
  }
}

void WebServerManager::handleWebSocketEvent(uint8_t clientNum, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      LINK_LOGD(LOG_TAG, "WebSocket client #%u disconnected", clientNum);
      break;
      
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(clientNum);
      LINK_LOGI(LOG_TAG, "WebSocket client #%u connected from %s", clientNum, ip.toString().c_str());
      
      // Send initial data to the newly connected client
      sendLogData(clientNum);
      sendTrainData(clientNum);
      sendLEDState(clientNum);
      break;
    }
      
    case WStype_TEXT:
      // Handle incoming text messages
      LINK_LOGD(LOG_TAG, "WebSocket message from client #%u: %s", clientNum, payload);
      {
        JsonDocument doc(PSRAMJsonAllocator::instance());
        DeserializationError error = deserializeJson(doc, payload, length);
        if (!error) {
          const char* type = doc["type"] | "";
          if (strcmp(type, "setFocus") == 0) {
            const char* vehicleId = doc["vehicleId"] | "";
            preferencesManager.setFocusedVehicleId(String(vehicleId));
            LINK_LOGD(LOG_TAG, "Focused vehicle ID set to: %s", vehicleId);
          }
        }
      }
      break;
      
    case WStype_ERROR:
      LINK_LOGE(LOG_TAG, "WebSocket error on client #%u", clientNum);
      break;
      
    default:
      break;
  }
}

void WebServerManager::broadcastLog(const LogEntry& entry) {
  if (webSocket.connectedClients() == 0) {
    return;
  }
  
  String jsonResponse;
  logManager.getLogEntryAsJson(entry, jsonResponse);
  webSocket.broadcastTXT(jsonResponse);
}

void WebServerManager::sendLogData(int clientNum) {
  String jsonResponse;
  logManager.getLogsAsJson(jsonResponse, "initial");
  if (clientNum == -1) {
    if (webSocket.connectedClients() == 0) {
      return;
    }
    webSocket.broadcastTXT(jsonResponse);
  } else {
    webSocket.sendTXT(static_cast<uint8_t>(clientNum), jsonResponse);
  }
}

void WebServerManager::sendLEDState(int clientNum) {
  String jsonResponse;
  ledController.getLEDStateAsJson(jsonResponse);
  if (clientNum == -1) {
    if (webSocket.connectedClients() == 0) {
      return;
    }
    webSocket.broadcastTXT(jsonResponse);
  } else {
    webSocket.sendTXT(static_cast<uint8_t>(clientNum), jsonResponse);
  }
}
