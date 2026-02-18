/*
#2  0x42004a32 in LogManager::addLog(char const*, char const*, char const*) at src/LogManager.cpp:28 (discriminator 1)
#3  0x42004bc5 in LogManager::log(char const*, char const*, char const*, ...) at src/LogManager.cpp:20
#4  0x42008672 in TrainDataManager::parseTrainDataFromJson(ArduinoJson::V742PB22::JsonDocument&, String const&) at src/TrainDataManager.cpp:156 (discriminator 35)
#5  0x42008eb9 in TrainDataManager::fetchTrainDataForRoute(String const&, String const&, String const&) at src/TrainDataManager.cpp:197
#6  0x420095ea in TrainDataManager::updateTrainPositions() at src/TrainDataManager.cpp:244 (discriminator 3)
  */
#include "LogManager.h"

static const char* TAG = "LogManager";

LogManager logManager;

void LogManager::setup() {
  // Buffer is ready to receive logs
  String msg = "Log manager initialized with buffer size " + String(LOG_BUFFER_SIZE);
  addLog("I", TAG, msg.c_str());
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
    
    logBuffer.push_back(entry);
    
    // Keep only the most recent LOG_BUFFER_SIZE entries
    // Using deque allows O(1) pop_front instead of O(n) erase(begin())
    if (logBuffer.size() > LOG_BUFFER_SIZE) {
      logBuffer.pop_front();
    }
  } catch (...) {
    Serial.println("[LogManager] Exception in addLog - likely memory allocation failure");
  }
}

std::deque<LogEntry, PSRAMAllocator<LogEntry>> LogManager::getLogs(int maxEntries) {
  // Validate maxEntries to prevent undefined behavior from negative values
  if (maxEntries <= 0) {
    maxEntries = LOG_BUFFER_SIZE;
  }
  
  size_t startIndex = 0;
  if (logBuffer.size() > (size_t)maxEntries) {
    startIndex = logBuffer.size() - maxEntries;
  }
  
  std::deque<LogEntry, PSRAMAllocator<LogEntry>> result;
  for (size_t i = startIndex; i < logBuffer.size(); i++) {
    result.push_back(logBuffer[i]);
  }
  
  return result;
}

void LogManager::clear() {
  logBuffer.clear();
  addLog("I", TAG, "Log buffer cleared");
}
