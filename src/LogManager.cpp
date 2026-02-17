#include "LogManager.h"

static const char* TAG = "LogManager";

LogManager logManager;

void LogManager::setup() {
  // Buffer is ready to receive logs
  String msg = "Log manager initialized with buffer size " + String(LOG_BUFFER_SIZE);
  addLog("I", TAG, msg.c_str());
}

void LogManager::log(const char* level, const char* tag, const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  addLog(level, tag, buffer);
}

void LogManager::addLog(const char* level, const char* tag, const char* message) {
  LogEntry entry;
  entry.timestamp = millis();
  entry.level = String(level);
  entry.tag = String(tag);
  entry.message = String(message);
  
  logBuffer.push_back(entry);
  
  // Keep only the most recent LOG_BUFFER_SIZE entries
  // Using deque allows O(1) pop_front instead of O(n) erase(begin())
  if (logBuffer.size() > LOG_BUFFER_SIZE) {
    logBuffer.pop_front();
  }
}

std::deque<LogEntry> LogManager::getLogs(int maxEntries) {
  // Validate maxEntries to prevent undefined behavior from negative values
  if (maxEntries <= 0) {
    maxEntries = LOG_BUFFER_SIZE;
  }
  
  size_t startIndex = 0;
  if (logBuffer.size() > (size_t)maxEntries) {
    startIndex = logBuffer.size() - maxEntries;
  }
  
  std::deque<LogEntry> result;
  for (size_t i = startIndex; i < logBuffer.size(); i++) {
    result.push_back(logBuffer[i]);
  }
  
  return result;
}

void LogManager::clear() {
  logBuffer.clear();
  addLog("I", TAG, "Log buffer cleared");
}
