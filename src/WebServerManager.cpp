#include "WebServerManager.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Ministache.h>
#include <esp_log.h>
#include "FileSystemManager.h"
#include "PreferencesManager.h"

static const char* TAG = "WebServerManager";

WebServerManager webServerManager;

void WebServerManager::setup() {
  ESP_LOGI(TAG, "Setting up web server...");
  
  // Register handlers
  server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server.on("/config", HTTP_GET, [this]() { this->handleConfig(); });
  server.on("/config", HTTP_POST, [this]() { this->handleSaveConfig(); });
  
  // Start server
  server.begin();
  ESP_LOGI(TAG, "Web server started");
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
  // Only set homeStation if not empty, so Mustache {{^homeStation}} works
  String homeStation = preferencesManager.getHomeStation();
  if (!homeStation.isEmpty()) {
    data["homeStation"] = homeStation;
  }
  
  String output = ministache::render(html, data);
  server.send(200, "text/html", output);

  ESP_LOGI(TAG, "Served root page");
}

void WebServerManager::handleConfig() {
  String html = fileSystemManager.readFile("/config.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load config.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  // Create data for Ministache template
  JsonDocument data;
  data["homeStation"] = preferencesManager.getHomeStation();
  data["apiKey"] = preferencesManager.getApiKey();
  data["routeId"] = preferencesManager.getRouteId();
  
  String output = ministache::render(html, data);
  server.send(200, "text/html", output);
}

void WebServerManager::handleSaveConfig() {
  // Validate and sanitize inputs
  if (server.hasArg("homeStation")) {
    String homeStation = server.arg("homeStation");
    // Limit length to prevent excessive storage use
    if (homeStation.length() > 64) {
      homeStation = homeStation.substring(0, 64);
    }
    preferencesManager.setHomeStation(homeStation);
  }
  if (server.hasArg("apiKey")) {
    String apiKey = server.arg("apiKey");
    // Limit length to prevent excessive storage use
    if (apiKey.length() > 64) {
      apiKey = apiKey.substring(0, 64);
    }
    preferencesManager.setApiKey(apiKey);
  }
  if (server.hasArg("routeId")) {
    String newRouteId = server.arg("routeId");
    if (newRouteId.length() > 0) {
      // Limit length to prevent excessive storage use
      if (newRouteId.length() > 32) {
        newRouteId = newRouteId.substring(0, 32);
      }
      preferencesManager.setRouteId(newRouteId);
    } else {
      // If routeId is empty, use default
      preferencesManager.setRouteId(DEFAULT_ROUTE_ID);
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
