#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <WebServer.h>
#include <WebSocketsServer.h>
#include "config.h"

class WebServerManager {
public:
  void setup();
  void handleClient();
  void broadcastLog(const char* level, const char* tag, const char* message, unsigned long timestamp);
  void broadcastTrainData();
  void broadcastLEDState();
  
private:
  void handleRoot();
  void handleConfig();
  void handleSaveConfig();
  void handleTestStation();
  void handleLogs();
  void handleLogsData();
  void handleTrains();
  void handleWebSocketEvent(uint8_t clientNum, WStype_t type, uint8_t * payload, size_t length);
  void sendTrainData(uint8_t clientNum);
  void sendLEDState(uint8_t clientNum);
  
  WebServer server{WEB_SERVER_PORT};
  WebSocketsServer webSocket{WEB_SOCKET_PORT};
};

extern WebServerManager webServerManager;

#endif // WEBSERVERMANAGER_H
