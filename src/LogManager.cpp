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
  // Make a copy of args for our use since va_list can only be used once
  va_list argsCopy;
  va_copy(argsCopy, args);
  
  // First, output to serial as normal
  int ret = vprintf(format, args);
  
  // Parse the log message using our copy
  // ESP_LOG format: "level (timestamp) tag: message"
  // Example: "I (12345) WebServerManager: Setting up web server..."
  
  char buffer[512];
  vsnprintf(buffer, sizeof(buffer), format, argsCopy);
  va_end(argsCopy);
  
  String logLine = String(buffer);
  Serial.printf("[customVprintf] Raw buffer: %s\n", buffer);
  Serial.printf("[customVprintf] logLine.length()=%d, instance=%p\n", logLine.length(), instance);
  
  // Only process lines that start with a log level indicator
  if (logLine.length() > 0 && instance != nullptr) {
    char level = logLine.charAt(0);
    Serial.printf("[customVprintf] First char: '%c'\n", level);
    
    // Check if this is a log line (starts with level character)
    if (level == 'E' || level == 'W' || level == 'I' || level == 'D' || level == 'V') {
      Serial.println("[customVprintf] Valid log level detected");
      // Extract tag and message
      int tagStart = logLine.indexOf(") ") + 2;
      int tagEnd = logLine.indexOf(": ", tagStart);
      Serial.printf("[customVprintf] tagStart=%d, tagEnd=%d\n", tagStart, tagEnd);
      
      if (tagStart > 2 && tagEnd > tagStart) {
        String levelStr = String(level);
        String tag = logLine.substring(tagStart, tagEnd);
        String message = logLine.substring(tagEnd + 2);
        
        // Remove trailing newline
        message.trim();
        
        Serial.printf("[customVprintf] Adding log: level=%s, tag=%s, message=%s\n", 
          levelStr.c_str(), tag.c_str(), message.c_str());
        instance->addLog(levelStr.c_str(), tag.c_str(), message.c_str());
      } else {
        Serial.println("[customVprintf] tagStart or tagEnd validation failed");
      }
    } else {
      Serial.printf("[customVprintf] Invalid log level: '%c'\n", level);
    }
  } else {
    Serial.println("[customVprintf] logLine empty or instance is nullptr");
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
  
  ESP_LOGI(TAG, "getLogs called: maxEntries=%d, bufferSize=%d, startIndex=%d, returning %d entries", 
    maxEntries, logBuffer.size(), startIndex, (logBuffer.size() - startIndex));
  
  std::deque<LogEntry> result;
  for (size_t i = startIndex; i < logBuffer.size(); i++) {
    result.push_back(logBuffer[i]);
  }
  
  return result;
}

void LogManager::clear() {
  logBuffer.clear();
  ESP_LOGI(TAG, "Log buffer cleared");
}
