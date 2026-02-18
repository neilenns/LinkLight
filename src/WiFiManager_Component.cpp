#include "WiFiManager_Component.h"
#include <WiFi.h>
#include "LogManager.h"
#include "PreferencesManager.h"
#include "config.h"

static const char* LOG_TAG = "WiFiManager_Component";

WiFiManager_Component wifiManagerComponent;

void WiFiManager_Component::setup() {
  LINK_LOGI(LOG_TAG, "Setting up WiFi...");
  
  // Set hostname for the device (must be set before WiFi.mode)
  String hostname = preferencesManager.getHostname();
  WiFi.setHostname(hostname.c_str());
  LINK_LOGI(LOG_TAG, "Hostname set to: %s", hostname.c_str());
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  
  // Configure WiFiManager
  wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);
  
  // Try to connect to WiFi
  if (!wifiManager.autoConnect("LinkLight-Setup")) {
    LINK_LOGE(LOG_TAG, "Failed to connect to WiFi");
    delay(3000);
    ESP.restart();
  }
  
  LINK_LOGI(LOG_TAG, "Connected to WiFi");
  LINK_LOGI(LOG_TAG, "IP Address: %s", WiFi.localIP().toString().c_str());
}
