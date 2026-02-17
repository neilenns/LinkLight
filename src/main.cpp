#include <Arduino.h>
#include <WiFi.h>
#include <esp_log.h>
#include "config.h"
#include "WiFiManager_Component.h"
#include "OTAManager.h"
#include "WebServerManager.h"
#include "LEDController.h"
#include "PreferencesManager.h"
#include "FileSystemManager.h"
#include "TrainDataManager.h"

static const char* TAG = "LinkLight";
unsigned long lastApiUpdate = 0;

void setup() {
  Serial.begin(115200);
  
  delay(5000);

  ESP_LOGI(TAG, "LinkLight Starting...");

  // Initialize LEDs first for visual feedback
  ledController.setup();
  
  // Initialize LittleFS
  fileSystemManager.setup();
  
  // Load saved preferences
  preferencesManager.load();
  
  // Setup WiFi
  wifiManagerComponent.setup();
  
  // Setup OTA
  otaManager.setup();
  
  // Setup web server
  webServerManager.setup();
  
  ESP_LOGI(TAG, "LinkLight Ready!");
  ESP_LOGI(TAG, "IP Address: %s", WiFi.localIP().toString().c_str());
}

void loop() {
  // Handle OTA updates
  otaManager.handle();
  
  // Handle web server requests
  webServerManager.handleClient();
  
  // Update train positions periodically
  if (millis() - lastApiUpdate > API_UPDATE_INTERVAL) {
    trainDataManager.updateTrainPositions();
    
    // Display current train positions on LEDs
    ledController.displayTrainPositions();

    lastApiUpdate = millis();
  }
    
  delay(10);
}

