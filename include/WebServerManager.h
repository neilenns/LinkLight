#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <WebServer.h>
#include "config.h"

class WebServerManager {
public:
  void setup();
  void handleClient();
  
private:
  void handleRoot();
  void handleConfig();
  void handleSaveConfig();
  
  WebServer server{WEB_SERVER_PORT};
};

extern WebServerManager webServerManager;

#endif // WEBSERVERMANAGER_H
