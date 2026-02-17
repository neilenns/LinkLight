#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <Arduino.h>
#include <deque>
#include <esp_log.h>

#define LOG_BUFFER_SIZE 100  // Number of log entries to keep in memory

struct LogEntry {
  unsigned long timestamp;  // millis() when log was created
  String level;             // "I", "W", "E", "D", "V"
  String tag;               // Component tag (e.g., "WebServerManager")
  String message;           // Log message
};

class LogManager {
public:
  void setup();
  void addLog(const char* level, const char* tag, const char* message);
  void log(const char* level, const char* tag, const char* format, ...);
  std::deque<LogEntry> getLogs(int maxEntries = LOG_BUFFER_SIZE);
  void clear();
  
private:
  std::deque<LogEntry> logBuffer;
};

extern LogManager logManager;

// Custom logging macros that capture to buffer AND output to serial
// These replace ESP_LOG macros to ensure logs are captured
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#define LINK_LOG(level, tag, format, ...) do { \
  ESP_LOG##level(tag, format, ##__VA_ARGS__); \
  logManager.log(#level, tag, format, ##__VA_ARGS__); \
} while(0)

#define LINK_LOGI(tag, format, ...) LINK_LOG(I, tag, format, ##__VA_ARGS__)
#define LINK_LOGW(tag, format, ...) LINK_LOG(W, tag, format, ##__VA_ARGS__)
#define LINK_LOGE(tag, format, ...) LINK_LOG(E, tag, format, ##__VA_ARGS__)
#define LINK_LOGD(tag, format, ...) LINK_LOG(D, tag, format, ##__VA_ARGS__)
#define LINK_LOGV(tag, format, ...) LINK_LOG(V, tag, format, ##__VA_ARGS__)

#endif // LOGMANAGER_H
