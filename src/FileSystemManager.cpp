#include "FileSystemManager.h"
#include <LittleFS.h>
#include "LogManager.h"

static const char* LOG_TAG = "FileSystemManager";

FileSystemManager fileSystemManager;

void FileSystemManager::setup() {
  LINK_LOGD(LOG_TAG, "Setting up LittleFS...");
  
  if (!LittleFS.begin(true)) {
    LINK_LOGE(LOG_TAG, "LittleFS mount failed - web interface will not work");
    return;
  }
  }
