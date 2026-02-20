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
#include "NTPManager.h"

static const char* LOG_TAG = "LinkLight";
static TaskHandle_t loopTaskHandle = nullptr;

void dumpMemoryStats()
{
  if (psramFound())
  {
    LINK_LOGD(LOG_TAG, "Total PSRAM size: %lu kB\r\n", ESP.getPsramSize() / 1024);
    LINK_LOGD(LOG_TAG, "Free PSRAM size: %lu kB\r\n", ESP.getFreePsram() / 1024);
    LINK_LOGD(LOG_TAG, "Available internal heap: %lu kB\n", ESP.getFreeHeap() / 1024);
  }
  else
  {
    LINK_LOGW(LOG_TAG, "No PSRAM found");
  }
}

void trainUpdateTask(void* parameter) {
  TaskHandle_t notifyTarget = static_cast<TaskHandle_t>(parameter);
  while (true) {
    dumpMemoryStats();

    trainDataManager.updateTrainPositions();

    dumpMemoryStats();

    xTaskNotifyGive(notifyTarget);

    unsigned long updateIntervalMs = preferencesManager.getUpdateInterval() * 1000;
    vTaskDelay(pdMS_TO_TICKS(updateIntervalMs));
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize log manager early to capture all logs
  logManager.setup();

  // Initialize LEDs
  ledController.setup();

  // Initialize LittleFS
  fileSystemManager.setup();
  
  // Load saved preferences
  preferencesManager.load();
  
  // Setup WiFi
  wifiManagerComponent.setup();

  // Setup NTP time synchronization
  ntpManager.setup();

  // Setup OTA
  otaManager.setup();
  
  // Setup web server
  webServerManager.setup();
  
  // Show the startup animation
  ledController.startupAnimation();

  // Initialize the train data mutex
  trainDataManager.dataMutex = xSemaphoreCreateMutex();
  if (trainDataManager.dataMutex == nullptr) {
    LINK_LOGE(LOG_TAG, "Failed to create train data mutex");
    return;
  }

  // Capture the loop task handle so the Core 0 task can notify it
  loopTaskHandle = xTaskGetCurrentTaskHandle();

  // Start the train update task on Core 0, passing the loop task handle for notifications
  if (xTaskCreatePinnedToCore(trainUpdateTask, "TrainUpdate", 8192, loopTaskHandle, 1, NULL, 0) != pdPASS) {
    LINK_LOGE(LOG_TAG, "Failed to create train update task");
    return;
  }

  LINK_LOGI(LOG_TAG, "LinkLight Ready!");
}

void loop() {
  // Handle OTA updates
  otaManager.handle();
  
  // Handle web server requests
  webServerManager.handleClient();
  
  // When the Core 0 train update task signals new data is ready, broadcast and display it
  if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
    // Broadcast updated train data to connected WebSocket clients
    webServerManager.sendTrainData();
    
    // Display current train positions on LEDs
    ledController.displayTrainPositions();
    
    // Broadcast updated LED state to connected WebSocket clients
    webServerManager.sendLEDState();
  }
    
  delay(10);
}

