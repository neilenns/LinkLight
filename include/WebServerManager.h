#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "LogManager.h"
#include "TrainDataManager.h"

class WebServerManager {
public:
  void setup();
  void handleClient();
  void broadcastLog(const LogEntry& entry);
  void sendLogData(int clientNum = -1);
  void sendTrainData(int clientNum = -1);
  void sendLEDState(int clientNum = -1);
  
private:
  void handleRoot();
  void handleConfig();
  void handleSaveConfig();
  void handleTestStation();
  void handleLogs();
  void handleLogsData();
  void handleTrains();
  void handleWebSocketEvent(uint8_t clientNum, WStype_t type, uint8_t * payload, size_t length);
  
  WebServer server{WEB_SERVER_PORT};
  WebSocketsServer webSocket{WEB_SOCKET_PORT};
};

extern WebServerManager webServerManager;

#endif // WEBSERVERMANAGER_H
