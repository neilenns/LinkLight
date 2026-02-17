#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <esp_log.h>
#include "config.h"
#include "WiFiManager_Component.h"
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
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (strlen(OTA_PASSWORD) > 0) {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS or U_LittleFS
      type = "filesystem";
    }
    ESP_LOGI(TAG, "Start updating %s", type.c_str());
  });
  
  ArduinoOTA.onEnd([]() {
    ESP_LOGI(TAG, "OTA Update Complete");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    ESP_LOGI(TAG, "Progress: %u%%", (progress * 100) / total);
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    ESP_LOGE(TAG, "OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) ESP_LOGE(TAG, "Auth Failed");
    else if (error == OTA_BEGIN_ERROR) ESP_LOGE(TAG, "Begin Failed");
    else if (error == OTA_CONNECT_ERROR) ESP_LOGE(TAG, "Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) ESP_LOGE(TAG, "Receive Failed");
    else if (error == OTA_END_ERROR) ESP_LOGE(TAG, "End Failed");
  });
  
  ArduinoOTA.begin();
  ESP_LOGI(TAG, "OTA Ready");
  
  // Setup web server
  webServerManager.setup();
  
  ESP_LOGI(TAG, "LinkLight Ready!");
  ESP_LOGI(TAG, "IP Address: %s", WiFi.localIP().toString().c_str());
}

void loop() {
  // Handle OTA updates
  ArduinoOTA.handle();
  
  // Handle web server requests
  webServerManager.handleClient();
  
  // Update train positions periodically
  if (millis() - lastApiUpdate > API_UPDATE_INTERVAL) {
    trainDataManager.updateTrainPositions();
    lastApiUpdate = millis();
  }
  
  // Display current train positions on LEDs
  ledController.displayTrainPositions();
  
  delay(10);
}

