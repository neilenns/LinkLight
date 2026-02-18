#ifndef PSRAMJSONALLOCATOR_H
#define PSRAMJSONALLOCATOR_H

#include <ArduinoJson.h>
#include <esp_heap_caps.h>

/**
 * @brief Custom ArduinoJson allocator that uses ESP32's PSRAM
 * 
 * This allocator ensures that all JSON document memory is allocated
 * from PSRAM instead of regular heap, helping to prevent out-of-memory
 * crashes when processing large JSON documents (e.g., 73KB API responses).
 */
class PSRAMJsonAllocator : public ArduinoJson::Allocator {
public:
  void* allocate(size_t size) override {
    // Try to allocate from PSRAM first
    void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    
    // Fall back to regular heap if PSRAM allocation fails
    if (ptr == nullptr) {
      ptr = malloc(size);
    }
    
    return ptr;
  }

  void deallocate(void* ptr) override {
    // heap_caps_free works for both PSRAM and regular heap
    heap_caps_free(ptr);
  }

  void* reallocate(void* ptr, size_t new_size) override {
    // Try to reallocate in PSRAM first
    void* new_ptr = heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
    
    // Fall back to regular heap if PSRAM reallocation fails
    if (new_ptr == nullptr && new_size > 0) {
      new_ptr = realloc(ptr, new_size);
    }
    
    return new_ptr;
  }

  static PSRAMJsonAllocator* instance() {
    static PSRAMJsonAllocator allocator;
    return &allocator;
  }

private:
  PSRAMJsonAllocator() = default;
  ~PSRAMJsonAllocator() = default;
};

#endif // PSRAMJSONALLOCATOR_H
