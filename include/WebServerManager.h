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
  void handleLogs();
  void handleLogsData();
  
  bool isValidHexColor(const String& color);
  
  WebServer server{WEB_SERVER_PORT};
};

extern WebServerManager webServerManager;

#endif // WEBSERVERMANAGER_H
