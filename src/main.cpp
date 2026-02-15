#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <NeoPixelBus.h>
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
void loadPreferences();
void savePreferences();
void handleRoot();
void handleConfig();
void handleSaveConfig();
void updateTrainPositions();
void displayTrainPositions();

void setup() {
  Serial.begin(115200);
  Serial.println("\nLinkLight Starting...");

  // Initialize LEDs first for visual feedback
  setupLEDs();
  
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

void loadPreferences() {
  Serial.println("Loading preferences...");
  
  preferences.begin(PREF_NAMESPACE, false);
  
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

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>LinkLight</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += "h1 { color: #0066cc; }";
  html += "a { display: inline-block; margin: 10px 0; padding: 10px 20px; ";
  html += "background-color: #0066cc; color: white; text-decoration: none; border-radius: 4px; }";
  html += "a:hover { background-color: #0052a3; }";
  html += ".info { margin: 20px 0; padding: 10px; background-color: #f0f0f0; border-radius: 4px; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>LinkLight</h1>";
  html += "<div class='info'>";
  html += "<p><strong>Status:</strong> Running</p>";
  html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
  html += "<p><strong>Home Station:</strong> " + (homeStation.isEmpty() ? "Not configured" : homeStation) + "</p>";
  html += "</div>";
  html += "<a href='/config'>Configuration</a>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleConfig() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>LinkLight Configuration</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += "h1 { color: #0066cc; }";
  html += "form { max-width: 400px; }";
  html += "label { display: block; margin: 10px 0 5px; font-weight: bold; }";
  html += "input { width: 100%; padding: 8px; margin-bottom: 15px; box-sizing: border-box; }";
  html += "button { padding: 10px 20px; background-color: #0066cc; color: white; ";
  html += "border: none; border-radius: 4px; cursor: pointer; }";
  html += "button:hover { background-color: #0052a3; }";
  html += "a { display: inline-block; margin-top: 20px; color: #0066cc; text-decoration: none; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>LinkLight Configuration</h1>";
  html += "<form method='POST' action='/config'>";
  html += "<label>Home Station:</label>";
  html += "<input type='text' name='homeStation' value='" + homeStation + "' placeholder='Enter station name'>";
  html += "<label>API Key:</label>";
  html += "<input type='text' name='apiKey' value='" + apiKey + "' placeholder='Enter OneBusAway API key'>";
  html += "<label>Route ID:</label>";
  html += "<input type='text' name='routeId' value='" + routeId + "' placeholder='Route ID'>";
  html += "<button type='submit'>Save Configuration</button>";
  html += "</form>";
  html += "<a href='/'>Back to Home</a>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleSaveConfig() {
  if (server.hasArg("homeStation")) {
    homeStation = server.arg("homeStation");
  }
  if (server.hasArg("apiKey")) {
    apiKey = server.arg("apiKey");
  }
  if (server.hasArg("routeId")) {
    routeId = server.arg("routeId");
  }
  
  savePreferences();
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Configuration Saved</title>";
  html += "<meta http-equiv='refresh' content='2;url=/' />";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; text-align: center; }";
  html += "h1 { color: #0066cc; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>Configuration Saved!</h1>";
  html += "<p>Redirecting to home page...</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void updateTrainPositions() {
  if (apiKey.isEmpty()) {
    Serial.println("API key not configured");
    return;
  }
  
  Serial.println("Updating train positions...");
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/trips-for-route/" + routeId + ".json?" + API_KEY_PARAM + "=" + apiKey;
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    // Parse JSON response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
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
  static uint8_t hue = 0;
  
  for (int i = 0; i < LED_COUNT; i++) {
    strip.SetPixelColor(i, HslColor(hue / 255.0f, 1.0f, 0.1f));
  }
  
  strip.Show();
  hue++;
}
