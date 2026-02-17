#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <Arduino.h>
#include <vector>

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
  std::vector<LogEntry> getLogs(int maxEntries = LOG_BUFFER_SIZE);
  void clear();
  
private:
  std::vector<LogEntry> logBuffer;
  static int customVprintf(const char* format, va_list args);
  static LogManager* instance;
};

extern LogManager logManager;

#endif // LOGMANAGER_H
