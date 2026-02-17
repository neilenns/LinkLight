#include "LogManager.h"
#include <esp_log.h>

static const char* TAG = "LogManager";

LogManager logManager;
LogManager* LogManager::instance = nullptr;

void LogManager::setup() {
  instance = this;
  ESP_LOGI(TAG, "Log manager initialized with buffer size %d", LOG_BUFFER_SIZE);
  
  // Set custom vprintf handler to intercept ESP_LOG output
  esp_log_set_vprintf(customVprintf);
}

int LogManager::customVprintf(const char* format, va_list args) {
  // First, output to serial as normal
  int ret = vprintf(format, args);
  
  // Parse the log message
  // ESP_LOG format: "level (timestamp) tag: message"
  // Example: "I (12345) WebServerManager: Setting up web server..."
  
  char buffer[512];
  vsnprintf(buffer, sizeof(buffer), format, args);
  String logLine = String(buffer);
  
  // Only process lines that start with a log level indicator
  if (logLine.length() > 0 && instance != nullptr) {
    char level = logLine.charAt(0);
    
    // Check if this is a log line (starts with level character)
    if (level == 'E' || level == 'W' || level == 'I' || level == 'D' || level == 'V') {
      // Extract tag and message
      int tagStart = logLine.indexOf(") ") + 2;
      int tagEnd = logLine.indexOf(": ", tagStart);
      
      if (tagStart > 2 && tagEnd > tagStart) {
        String levelStr = String(level);
        String tag = logLine.substring(tagStart, tagEnd);
        String message = logLine.substring(tagEnd + 2);
        
        // Remove trailing newline
        message.trim();
        
        instance->addLog(levelStr.c_str(), tag.c_str(), message.c_str());
      }
    }
  }
  
  return ret;
}

void LogManager::addLog(const char* level, const char* tag, const char* message) {
  LogEntry entry;
  entry.timestamp = millis();
  entry.level = String(level);
  entry.tag = String(tag);
  entry.message = String(message);
  
  logBuffer.push_back(entry);
  
  // Keep only the most recent LOG_BUFFER_SIZE entries
  if (logBuffer.size() > LOG_BUFFER_SIZE) {
    logBuffer.erase(logBuffer.begin());
  }
}

std::vector<LogEntry> LogManager::getLogs(int maxEntries) {
  int startIndex = 0;
  if (logBuffer.size() > (size_t)maxEntries) {
    startIndex = logBuffer.size() - maxEntries;
  }
  
  std::vector<LogEntry> result;
  for (size_t i = startIndex; i < logBuffer.size(); i++) {
    result.push_back(logBuffer[i]);
  }
  
  return result;
}

void LogManager::clear() {
  logBuffer.clear();
  ESP_LOGI(TAG, "Log buffer cleared");
}
