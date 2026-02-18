#include "LogManager.h"

static const char* LOG_TAG = "LogManager";

LogManager logManager;

void LogManager::setup() {
  // Buffer is ready to receive logs
  String msg = "Log manager initialized with buffer size " + String(LOG_BUFFER_SIZE);
  addLog("I", LOG_TAG, msg.c_str());
}

void LogManager::log(const char* level, const char* tag, const char* format, ...) {
  char buffer[512];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  
  addLog(level, tag, buffer);
}

void LogManager::addLog(const char* level, const char* tag, const char* message) {
  // Validate inputs to prevent crashes
  if (!level || !tag || !message) {
    Serial.println("[LogManager] Null parameter passed to addLog");
    return;
  }
  
  try {
    LogEntry entry;
    entry.timestamp = millis();
    entry.level = String(level);
    entry.tag = String(tag);
    entry.message = String(message);
    
    // Ring buffer automatically overwrites oldest entry if full
    logBuffer.pushOverwrite(entry);
  } catch (...) {
    Serial.println("[LogManager] Exception in addLog - likely memory allocation failure");
  }
}

esp32_psram::VectorPSRAM<LogEntry> LogManager::getLogs(int maxEntries) {
  // Validate maxEntries to prevent undefined behavior from negative values
  if (maxEntries <= 0) {
    maxEntries = LOG_BUFFER_SIZE;
  }
  
  size_t available = logBuffer.available();
  size_t entriesToReturn = (available < (size_t)maxEntries) ? available : (size_t)maxEntries;
  
  esp32_psram::VectorPSRAM<LogEntry> result;
  result.reserve(entriesToReturn);
  
  // If we need fewer entries than available, skip the oldest ones
  size_t skipCount = (available > entriesToReturn) ? (available - entriesToReturn) : 0;
  
  for (size_t i = skipCount; i < available; i++) {
    LogEntry entry;
    if (logBuffer.peekAt(i, entry)) {
      result.push_back(entry);
    }
  }
  
  return result;
}

void LogManager::clear() {
  logBuffer.clear();
  addLog("I", LOG_TAG, "Log buffer cleared");
}
