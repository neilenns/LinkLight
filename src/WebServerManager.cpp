#include "WebServerManager.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Ministache.h>
#include "LogManager.h"
#include "FileSystemManager.h"
#include "PreferencesManager.h"
#include "LEDController.h"

static const char* TAG = "WebServerManager";

WebServerManager webServerManager;

void WebServerManager::setup() {
  LINK_LOGI(TAG, "Setting up web server...");
  
  // Register handlers
  server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server.on("/config", HTTP_GET, [this]() { this->handleConfig(); });
  server.on("/config", HTTP_POST, [this]() { this->handleSaveConfig(); });
  server.on("/logs", HTTP_GET, [this]() { this->handleLogs(); });
  server.on("/api/logs", HTTP_GET, [this]() { this->handleLogsData(); });
  
  // Start server
  server.begin();
  LINK_LOGI(TAG, "Web server started");
}

void WebServerManager::handleClient() {
  server.handleClient();
}

void WebServerManager::handleRoot() {
  String html = fileSystemManager.readFile("/index.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load index.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  // Create data for Ministache template
  JsonDocument data;
  data["ipAddress"] = WiFi.localIP().toString();
  data["hostname"] = preferencesManager.getHostname();
  
  String output = ministache::render(html, data);
  server.send(200, "text/html", output);

  LINK_LOGI(TAG, "Served root page");
}

void WebServerManager::handleConfig() {
  String html = fileSystemManager.readFile("/config.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load config.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  // Create data for Ministache template
  JsonDocument data;
  data["apiKey"] = preferencesManager.getApiKey();
  data["hostname"] = preferencesManager.getHostname();
  data["line1Color"] = preferencesManager.getLine1Color();
  data["line2Color"] = preferencesManager.getLine2Color();
  data["brightness"] = preferencesManager.getBrightness();
  
  String output = ministache::render(html, data);
  server.send(200, "text/html", output);
}

void WebServerManager::handleSaveConfig() {
  // Helper function to validate and sanitize hex color
  auto validateColor = [this](const String& color) -> String {
    String cleanColor = color;
    // Remove # if present
    if (cleanColor.startsWith("#")) {
      cleanColor = cleanColor.substring(1);
    }
    // Validate hex format
    if (cleanColor.length() == 6 && isValidHexColor(cleanColor)) {
      return cleanColor;
    }
    return ""; // Return empty string if invalid
  };
  
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
  if (server.hasArg("line1Color")) {
    String validColor = validateColor(server.arg("line1Color"));
    if (!validColor.isEmpty()) {
      preferencesManager.setLine1Color(validColor);
    }
  }
  if (server.hasArg("line2Color")) {
    String validColor = validateColor(server.arg("line2Color"));
    if (!validColor.isEmpty()) {
      preferencesManager.setLine2Color(validColor);
    }
  }
  if (server.hasArg("brightness")) {
    String brightnessStr = server.arg("brightness");
    int brightnessValue = brightnessStr.toInt();
    // Validate range 0-255
    if (brightnessValue >= 0 && brightnessValue <= 255) {
      preferencesManager.setBrightness((uint8_t)brightnessValue);
    }
  }
  
  preferencesManager.save();
  
  // Update LED colors after saving
  ledController.updateColors();
  
  String html = fileSystemManager.readFile("/config_saved.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load config_saved.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  server.send(200, "text/html", html);
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
  std::deque<LogEntry> logs = logManager.getLogs();
  
  // Create JSON response
  JsonDocument doc;
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

bool WebServerManager::isValidHexColor(const String& color) {
  for (size_t i = 0; i < color.length(); i++) {
    char c = color.charAt(i);
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
      return false;
    }
  }
  return true;
}
