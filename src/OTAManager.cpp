#include "OTAManager.h"
#include <ArduinoOTA.h>
#include "LogManager.h"
#include "PreferencesManager.h"
#include "config.h"

static const char* LOG_TAG = "OTAManager";

OTAManager otaManager;

void OTAManager::setup() {
  LINK_LOGD(LOG_TAG, "Setting up OTA...");
  
  // Set hostname from preferences
  String hostname = preferencesManager.getHostname();
  ArduinoOTA.setHostname(hostname.c_str());
  LINK_LOGI(LOG_TAG, "OTA hostname set to: %s", hostname.c_str());
  
  // Set password if configured
  if (strlen(OTA_PASSWORD) > 0) {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }
  
  // Configure callbacks
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS or U_LittleFS
      type = "filesystem";
    }
    LINK_LOGI(LOG_TAG, "Start updating %s", type.c_str());
  });
  
  ArduinoOTA.onEnd([]() {
    LINK_LOGI(LOG_TAG, "OTA Update Complete");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (total > 0) {
      LINK_LOGI(LOG_TAG, "Progress: %u%%", (progress * 100) / total);
    }
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    String errorMsg;
    if (error == OTA_AUTH_ERROR) errorMsg = "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) errorMsg = "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) errorMsg = "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) errorMsg = "Receive Failed";
    else if (error == OTA_END_ERROR) errorMsg = "End Failed";
    else errorMsg = "Unknown Error";
    LINK_LOGE(LOG_TAG, "OTA Error[%u]: %s", error, errorMsg.c_str());
  });
  
  // Start OTA service
  ArduinoOTA.begin();
  
  LINK_LOGD(LOG_TAG, "OTA Ready");
}

void OTAManager::handle() {
  ArduinoOTA.handle();
}
