#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <Arduino.h>
#include <deque>
#include <esp_log.h>
#include <esp_heap_caps.h>

#define LOG_BUFFER_SIZE 100  // Number of log entries to keep in memory

// Custom allocator that uses PSRAM instead of internal DRAM
// This prevents crashes when internal heap is fragmented or low
template <typename T>
class PSRAMAllocator {
public:
  using value_type = T;
  
  PSRAMAllocator() = default;
  
  template <typename U>
  PSRAMAllocator(const PSRAMAllocator<U>&) {}
  
  T* allocate(size_t n) {
    // Try PSRAM first (MALLOC_CAP_SPIRAM), fall back to internal if PSRAM unavailable
    void* ptr = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!ptr) {
      // PSRAM not available or full, try internal DRAM
      ptr = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    if (!ptr) {
      throw std::bad_alloc();
    }
    return static_cast<T*>(ptr);
  }
  
  void deallocate(T* ptr, size_t) {
    heap_caps_free(ptr);
  }
  
  template <typename U>
  bool operator==(const PSRAMAllocator<U>&) const { return true; }
  
  template <typename U>
  bool operator!=(const PSRAMAllocator<U>&) const { return false; }
};

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
  std::deque<LogEntry, PSRAMAllocator<LogEntry>> getLogs(int maxEntries = LOG_BUFFER_SIZE);
  void clear();
  
private:
  std::deque<LogEntry, PSRAMAllocator<LogEntry>> logBuffer;
};

extern LogManager logManager;

// Custom logging macros that capture to buffer AND output to serial
// These replace ESP_LOG macros to ensure logs are captured
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
