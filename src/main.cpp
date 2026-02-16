#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <NeoPixelBus.h>
#include <LittleFS.h>
#include <Ministache.h>
#include <esp_log.h>
#include <vector>
#include <map>
#include "config.h"

// Global objects
WiFiManager wifiManager;
WebServer server(WEB_SERVER_PORT);
Preferences preferences;
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> strip(LED_COUNT, LED_PIN);

// Global variables
String homeStation = "";
String apiKey = "";
String routeId = DEFAULT_ROUTE_ID;
unsigned long lastApiUpdate = 0;

static const char* TAG = "LinkLight";

// Train data structure
struct TrainData {
  String closestStop;
  int closestStopTimeOffset;
  String nextStop;
  int nextStopTimeOffset;
  String tripId;
  String directionId;
  String routeId;
  String tripHeadsign;
};

// Global train data storage
std::vector<TrainData> trainDataList;

// Function declarations
void setupWiFi();
void setupWebServer();
void setupLEDs();
void setupLittleFS();
void loadPreferences();
void savePreferences();
void handleRoot();
void handleConfig();
void handleSaveConfig();
void updateTrainPositions();
void displayTrainPositions();
String readFile(const char* path);

void setup() {
  Serial.begin(115200);
  
  delay(5000);

  ESP_LOGI(TAG, "LinkLight Starting...");

  // Initialize LEDs first for visual feedback
  setupLEDs();
  
  // Initialize LittleFS
  setupLittleFS();
  
  // Load saved preferences
  loadPreferences();
  
  // Setup WiFi
  setupWiFi();
  
  // Setup web server
  setupWebServer();
  
  ESP_LOGI(TAG, "LinkLight Ready!");
  ESP_LOGI(TAG, "IP Address: %s", WiFi.localIP().toString().c_str());
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Update train positions periodically
  if (millis() - lastApiUpdate > API_UPDATE_INTERVAL) {
    updateTrainPositions();
    lastApiUpdate = millis();
  }
  
  // Display current train positions on LEDs
  displayTrainPositions();
  
  delay(10);
}

void setupWiFi() {
  ESP_LOGI(TAG, "Setting up WiFi...");
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  
  // Configure WiFiManager
  wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);
  
  // Try to connect to WiFi
  if (!wifiManager.autoConnect("LinkLight-Setup")) {
    ESP_LOGE(TAG, "Failed to connect to WiFi");
    delay(3000);
    ESP.restart();
  }
  
  ESP_LOGI(TAG, "Connected to WiFi");
  ESP_LOGI(TAG, "IP Address: %s", WiFi.localIP().toString().c_str());
}

void setupWebServer() {
  ESP_LOGI(TAG, "Setting up web server...");
  
  // Register handlers
  server.on("/", HTTP_GET, handleRoot);
  server.on("/config", HTTP_GET, handleConfig);
  server.on("/config", HTTP_POST, handleSaveConfig);
  
  // Start server
  server.begin();
  ESP_LOGI(TAG, "Web server started");
}

void setupLEDs() {
  ESP_LOGI(TAG, "Setting up LEDs...");
  
  strip.Begin();
  strip.Show(); // Initialize all pixels to 'off'
  
  // Basic startup self-test: full-strip RGB flashes
  const RgbColor testColors[] = {
    RgbColor(32, 0, 0),
    RgbColor(0, 32, 0),
    RgbColor(0, 0, 32)
  };

  for (const auto& color : testColors) {
    for (int i = 0; i < LED_COUNT; i++) {
      strip.SetPixelColor(i, color);
    }
    strip.Show();
    delay(2000);
  }

  // Return LEDs to off state after test
  for (int i = 0; i < LED_COUNT; i++) {
    strip.SetPixelColor(i, RgbColor(0, 0, 0));
  }
  strip.Show();
  
  ESP_LOGI(TAG, "LEDs initialized");
}

void setupLittleFS() {
  ESP_LOGI(TAG, "Setting up LittleFS...");
  
  if (!LittleFS.begin(true)) {
    ESP_LOGE(TAG, "LittleFS mount failed - web interface will not work");
    return;
  }
  
  ESP_LOGI(TAG, "LittleFS mounted successfully");
}

void loadPreferences() {
  ESP_LOGI(TAG, "Loading preferences...");
  
  preferences.begin(PREF_NAMESPACE, true);
  
  homeStation = preferences.getString(PREF_HOME_STATION, "");
  apiKey = preferences.getString(PREF_API_KEY, "");
  routeId = preferences.getString(PREF_ROUTE_ID, DEFAULT_ROUTE_ID);
  
  preferences.end();
  
  ESP_LOGI(TAG, "Home Station: %s", homeStation.isEmpty() ? "Not set" : homeStation.c_str());
  ESP_LOGI(TAG, "Route ID: %s", routeId.c_str());
}

void savePreferences() {
  ESP_LOGI(TAG, "Saving preferences...");
  
  preferences.begin(PREF_NAMESPACE, false);
  
  preferences.putString(PREF_HOME_STATION, homeStation);
  preferences.putString(PREF_API_KEY, apiKey);
  preferences.putString(PREF_ROUTE_ID, routeId);
  
  preferences.end();
  
  ESP_LOGI(TAG, "Preferences saved");
}

String readFile(const char* path) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file: %s", path);
    return String();
  }
  
  String content = file.readString();
  file.close();
  
  return content;
}

void handleRoot() {
  String html = readFile("/index.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load index.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  // Create data for Ministache template
  JsonDocument data;
  data["ipAddress"] = WiFi.localIP().toString();
  // Only set homeStation if not empty, so Mustache {{^homeStation}} works
  if (!homeStation.isEmpty()) {
    data["homeStation"] = homeStation;
  }
  
  String output = ministache::render(html, data);
  server.send(200, "text/html", output);

  ESP_LOGI(TAG, "Served root page");
}

void handleConfig() {
  String html = readFile("/config.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load config.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  // Create data for Ministache template
  JsonDocument data;
  data["homeStation"] = homeStation;
  data["apiKey"] = apiKey;
  data["routeId"] = routeId;
  
  String output = ministache::render(html, data);
  server.send(200, "text/html", output);
}

void handleSaveConfig() {
  // Validate and sanitize inputs
  if (server.hasArg("homeStation")) {
    homeStation = server.arg("homeStation");
    // Limit length to prevent excessive storage use
    if (homeStation.length() > 64) {
      homeStation = homeStation.substring(0, 64);
    }
  }
  if (server.hasArg("apiKey")) {
    apiKey = server.arg("apiKey");
    // Limit length to prevent excessive storage use
    if (apiKey.length() > 64) {
      apiKey = apiKey.substring(0, 64);
    }
  }
  if (server.hasArg("routeId")) {
    String newRouteId = server.arg("routeId");
    if (newRouteId.length() > 0) {
      routeId = newRouteId;
      // Limit length to prevent excessive storage use
      if (routeId.length() > 32) {
        routeId = routeId.substring(0, 32);
      }
    } else {
      // If routeId is empty, use default
      routeId = DEFAULT_ROUTE_ID;
    }
  }
  
  savePreferences();
  
  String html = readFile("/config_saved.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load config_saved.html - ensure filesystem was uploaded with 'pio run --target uploadfs'");
    return;
  }
  
  server.send(200, "text/html", html);
}

void updateTrainPositions() {
  if (apiKey.isEmpty()) {
    ESP_LOGW(TAG, "API key not configured");
    return;
  }
  
  ESP_LOGI(TAG, "Updating train positions...");
  
  HTTPClient http;
  // Note: API key is passed as URL parameter per OneBusAway API specification
  // HTTPS is used to protect the key in transit
  String url = String(API_BASE_URL) + "/trips-for-route/" + routeId + ".json?" + API_KEY_PARAM + "=" + apiKey;
  
  http.setTimeout(10000); // 10 second timeout
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    // Parse JSON directly from stream to avoid double-buffering
    WiFiClient* stream = http.getStreamPtr();
    
    if (stream == nullptr) {
      ESP_LOGE(TAG, "Failed to get HTTP stream");
      http.end();
      return;
    }
    
    // Use JsonDocument for API response parsing
    // Sized for expected OneBusAway API response structure
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, *stream);
    
    if (error) {
      ESP_LOGE(TAG, "JSON parsing failed: %s", error.c_str());
    } else {
      ESP_LOGI(TAG, "Successfully retrieved train data");
      
      // Clear previous train data
      trainDataList.clear();
      
      // Get the data object
      JsonObject data = doc["data"];
      if (!data.isNull()) {
        // First, build a map of trip information
        std::map<String, JsonObject> tripMap;
        JsonArray trips = data["references"]["trips"];
        if (!trips.isNull()) {
          for (JsonObject trip : trips) {
            String tripId = trip["id"].as<String>();
            tripMap[tripId] = trip;
          }
          ESP_LOGI(TAG, "Loaded %d trip references", tripMap.size());
        }
        
        // Process the list array
        JsonArray list = data["list"];
        if (!list.isNull()) {
          for (JsonObject item : list) {
            TrainData train;
            
            // Extract tripId from the list item
            train.tripId = item["tripId"].as<String>();
            
            // Extract data from status object
            JsonObject status = item["status"];
            if (!status.isNull()) {
              train.closestStop = status["closestStop"].as<String>();
              train.closestStopTimeOffset = status["closestStopTimeOffset"].as<int>();
              train.nextStop = status["nextStop"].as<String>();
              train.nextStopTimeOffset = status["nextStopTimeOffset"].as<int>();
            }
            
            // Merge trip information if available
            if (tripMap.find(train.tripId) != tripMap.end()) {
              JsonObject tripInfo = tripMap[train.tripId];
              train.directionId = tripInfo["directionId"].as<String>();
              train.routeId = tripInfo["routeId"].as<String>();
              train.tripHeadsign = tripInfo["tripHeadsign"].as<String>();
            }
            
            // Add to the list
            trainDataList.push_back(train);
            
            // Log parsed data
            ESP_LOGI(TAG, "Train: tripId=%s, closestStop=%s, offset=%d, nextStop=%s, offset=%d, direction=%s, route=%s, headsign=%s",
              train.tripId.c_str(),
              train.closestStop.c_str(),
              train.closestStopTimeOffset,
              train.nextStop.c_str(),
              train.nextStopTimeOffset,
              train.directionId.c_str(),
              train.routeId.c_str(),
              train.tripHeadsign.c_str());
          }
          ESP_LOGI(TAG, "Processed %d train positions", trainDataList.size());
        }
      }
    }
  } else {
    ESP_LOGW(TAG, "HTTP request failed: %d", httpCode);
  }
  
  http.end();
}

void displayTrainPositions() {
  // TODO: Implement LED display logic based on train positions
  // For now, just show a simple pattern to indicate the system is working
  static uint8_t animationHue = 0;
  
  for (int i = 0; i < LED_COUNT; i++) {
    strip.SetPixelColor(i, HslColor(animationHue / 255.0f, 1.0f, 0.1f));
  }
  
  strip.Show();
  animationHue++;
}
