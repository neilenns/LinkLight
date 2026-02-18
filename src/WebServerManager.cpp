#include "WebServerManager.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Ministache.h>
#include "LogManager.h"
#include "FileSystemManager.h"
#include "PreferencesManager.h"
#include "PSRAMJsonAllocator.h"

static const char* LOG_TAG = "WebServerManager";

WebServerManager webServerManager;

void WebServerManager::setup() {
  LINK_LOGI(LOG_TAG, "Setting up web server...");
  
  // Register handlers
  server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server.on("/config", HTTP_GET, [this]() { this->handleConfig(); });
  server.on("/config", HTTP_POST, [this]() { this->handleSaveConfig(); });
  server.on("/logs", HTTP_GET, [this]() { this->handleLogs(); });
  server.on("/api/logs", HTTP_GET, [this]() { this->handleLogsData(); });
  
  // Start server
  server.begin();
  LINK_LOGI(LOG_TAG, "Web server started");
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
  
  preferencesManager.save();
  
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
