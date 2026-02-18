#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "LogManager.h"
#include "WiFiManager_Component.h"
#include "OTAManager.h"
#include "WebServerManager.h"
#include "LEDController.h"
#include "PreferencesManager.h"
#include "FileSystemManager.h"
#include "TrainDataManager.h"

static const char* TAG = "LinkLight";
unsigned long lastApiUpdate = -API_UPDATE_INTERVAL; // Initial value is a really large number to ensure the first update happens immediately.

void setup() {
  Serial.begin(115200);

  // Initialize log manager early to capture all logs
  logManager.setup();

  LINK_LOGI(TAG, "LinkLight Starting...");
  
  // Check PSRAM availability and heap sizes
  if (psramFound()) {
    LINK_LOGI(TAG, "PSRAM found: %d bytes total, %d bytes free", ESP.getPsramSize(), ESP.getFreePsram());
    LINK_LOGI(TAG, "Internal RAM heap: %d bytes free", ESP.getFreeHeap());
    LINK_LOGI(TAG, "Total free heap (RAM + PSRAM): %d bytes", ESP.getFreeHeap() + ESP.getFreePsram());
  } else {
    LINK_LOGW(TAG, "PSRAM not found!");
    LINK_LOGI(TAG, "Internal RAM heap: %d bytes free", ESP.getFreeHeap());
  }

  // Initialize LEDs
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
  
  // Show the startup animation
  ledController.startupAnimation();

  LINK_LOGI(TAG, "LinkLight Ready!");
  LINK_LOGI(TAG, "IP Address: %s", WiFi.localIP().toString().c_str());
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

