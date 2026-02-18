#include "FileSystemManager.h"
#include <LittleFS.h>
#include "LogManager.h"

static const char* LOG_TAG = "FileSystemManager";

FileSystemManager fileSystemManager;

void FileSystemManager::setup() {
  LINK_LOGI(LOG_TAG, "Setting up LittleFS...");
  
  if (!LittleFS.begin(true)) {
    LINK_LOGE(LOG_TAG, "LittleFS mount failed - web interface will not work");
    return;
  }
  
  LINK_LOGI(LOG_TAG, "LittleFS mounted successfully");
}

String FileSystemManager::readFile(const char* path) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    LINK_LOGE(LOG_TAG, "Failed to open file: %s", path);
    return String();
  }
  
  String content = file.readString();
  file.close();
  
  return content;
}
