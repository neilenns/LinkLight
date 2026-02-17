#include "WiFiManager_Component.h"
#include <WiFi.h>
#include "LogManager.h"
#include "PreferencesManager.h"
#include "config.h"

static const char* TAG = "WiFiManager_Component";

WiFiManager_Component wifiManagerComponent;

void WiFiManager_Component::setup() {
  LINK_LOGI(TAG, "Setting up WiFi...");
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  
  // Set hostname for the device
  String hostname = preferencesManager.getHostname();
  WiFi.setHostname(hostname.c_str());
  LINK_LOGI(TAG, "Hostname set to: %s", hostname.c_str());
  
  // Configure WiFiManager
  wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);
  
  // Try to connect to WiFi
  if (!wifiManager.autoConnect("LinkLight-Setup")) {
    LINK_LOGE(TAG, "Failed to connect to WiFi");
    delay(3000);
    ESP.restart();
  }
  
  LINK_LOGI(TAG, "Connected to WiFi");
  LINK_LOGI(TAG, "IP Address: %s", WiFi.localIP().toString().c_str());
}
