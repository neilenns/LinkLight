#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <NeoPixelBus.h>
#include <LittleFS.h>
#include "config.h"

// Global objects
WiFiManager wifiManager;
WebServer server(WEB_SERVER_PORT);
Preferences preferences;
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(LED_COUNT, LED_PIN);

// Global variables
String homeStation = "";
String apiKey = "";
String routeId = DEFAULT_ROUTE_ID;
unsigned long lastApiUpdate = 0;

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
String escapeHtml(const String& str);
String readFile(const char* path);
String processTemplate(String html);

// Helper function to escape HTML entities
String escapeHtml(const String& str) {
  String escaped;
  escaped.reserve(str.length() * 2); // Pre-allocate to avoid multiple reallocations
  
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    switch (c) {
      case '&':  escaped += "&amp;"; break;
      case '<':  escaped += "&lt;"; break;
      case '>':  escaped += "&gt;"; break;
      case '"':  escaped += "&quot;"; break;
      case '\'': escaped += "&#39;"; break;
      default:   escaped += c; break;
    }
  }
  
  return escaped;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nLinkLight Starting...");

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
  
  Serial.println("LinkLight Ready!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
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
  Serial.println("Setting up WiFi...");
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  
  // Configure WiFiManager
  wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);
  
  // Try to connect to WiFi
  if (!wifiManager.autoConnect("LinkLight-Setup")) {
    Serial.println("Failed to connect to WiFi");
    delay(3000);
    ESP.restart();
  }
  
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setupWebServer() {
  Serial.println("Setting up web server...");
  
  // Register handlers
  server.on("/", HTTP_GET, handleRoot);
  server.on("/config", HTTP_GET, handleConfig);
  server.on("/config", HTTP_POST, handleSaveConfig);
  
  // Start server
  server.begin();
  Serial.println("Web server started");
}

void setupLEDs() {
  Serial.println("Setting up LEDs...");
  
  strip.Begin();
  strip.Show(); // Initialize all pixels to 'off'
  
  Serial.println("LEDs initialized");
}

void setupLittleFS() {
  Serial.println("Setting up LittleFS...");
  
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return;
  }
  
  Serial.println("LittleFS mounted successfully");
}

void loadPreferences() {
  Serial.println("Loading preferences...");
  
  preferences.begin(PREF_NAMESPACE, true);
  
  homeStation = preferences.getString(PREF_HOME_STATION, "");
  apiKey = preferences.getString(PREF_API_KEY, "");
  routeId = preferences.getString(PREF_ROUTE_ID, DEFAULT_ROUTE_ID);
  
  preferences.end();
  
  Serial.print("Home Station: ");
  Serial.println(homeStation.isEmpty() ? "Not set" : homeStation);
  Serial.print("Route ID: ");
  Serial.println(routeId);
}

void savePreferences() {
  Serial.println("Saving preferences...");
  
  preferences.begin(PREF_NAMESPACE, false);
  
  preferences.putString(PREF_HOME_STATION, homeStation);
  preferences.putString(PREF_API_KEY, apiKey);
  preferences.putString(PREF_ROUTE_ID, routeId);
  
  preferences.end();
  
  Serial.println("Preferences saved");
}

String readFile(const char* path) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.print("Failed to open file: ");
    Serial.println(path);
    return String();
  }
  
  String content = file.readString();
  file.close();
  
  return content;
}

String processTemplate(String html) {
  // Replace placeholders with actual values
  html.replace("%IP_ADDRESS%", WiFi.localIP().toString());
  
  // For display purposes (like home page), show "Not configured" if empty
  // For form inputs (like config page), use empty string to avoid pre-filling with "Not configured"
  if (html.indexOf("value='%HOME_STATION%'") != -1) {
    // Config form - use actual value (escaped), or empty if not set
    html.replace("%HOME_STATION%", escapeHtml(homeStation));
  } else {
    // Display text - show "Not configured" if empty
    html.replace("%HOME_STATION%", homeStation.isEmpty() ? "Not configured" : escapeHtml(homeStation));
  }
  
  html.replace("%API_KEY%", escapeHtml(apiKey));
  html.replace("%ROUTE_ID%", escapeHtml(routeId));
  
  return html;
}

void handleRoot() {
  String html = readFile("/index.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load index.html");
    return;
  }
  
  html = processTemplate(html);
  server.send(200, "text/html", html);
}

void handleConfig() {
  String html = readFile("/config.html");
  if (html.isEmpty()) {
    server.send(500, "text/plain", "Failed to load config.html");
    return;
  }
  
  html = processTemplate(html);
  server.send(200, "text/html", html);
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
    server.send(500, "text/plain", "Failed to load config_saved.html");
    return;
  }
  
  server.send(200, "text/html", html);
}

void updateTrainPositions() {
  if (apiKey.isEmpty()) {
    Serial.println("API key not configured");
    return;
  }
  
  Serial.println("Updating train positions...");
  
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
      Serial.println("Failed to get HTTP stream");
      http.end();
      return;
    }
    
    // Use JsonDocument for API response parsing
    // Sized for expected OneBusAway API response structure
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, *stream);
    
    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
    } else {
      Serial.println("Successfully retrieved train data");
      // TODO: Process train position data and update LED display
      // This will be implemented based on the actual API response structure
    }
  } else {
    Serial.print("HTTP request failed: ");
    Serial.println(httpCode);
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
