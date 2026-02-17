#ifndef WIFIMANAGER_COMPONENT_H
#define WIFIMANAGER_COMPONENT_H

#include <WiFiManager.h>

class WiFiManager_Component {
public:
  void setup();
  
private:
  WiFiManager wifiManager;
};

extern WiFiManager_Component wifiManagerComponent;

#endif // WIFIMANAGER_COMPONENT_H
