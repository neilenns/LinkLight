#include "FileSystemManager.h"
#include <LittleFS.h>
#include <esp_log.h>

static const char* TAG = "FileSystemManager";

FileSystemManager fileSystemManager;

void FileSystemManager::setup() {
  ESP_LOGI(TAG, "Setting up LittleFS...");
  
  if (!LittleFS.begin(true)) {
    ESP_LOGE(TAG, "LittleFS mount failed - web interface will not work");
    return;
  }
  
  ESP_LOGI(TAG, "LittleFS mounted successfully");
}

String FileSystemManager::readFile(const char* path) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file: %s", path);
    return String();
  }
  
  String content = file.readString();
  file.close();
  
  return content;
}
