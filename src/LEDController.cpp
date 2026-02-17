#include "LEDController.h"
#include <esp_log.h>

static const char* TAG = "LEDController";

LEDController ledController;

void LEDController::setup() {
  ESP_LOGI(TAG, "Setting up LEDs...");
  
  strip.Begin();
  strip.Show(); // Initialize all pixels to 'off'
  
  // Basic startup self-test: full-strip RGB flashes
  const RgbColor testColors[] = {
    RgbColor(32, 0, 0),
    RgbColor(0, 32, 0),
    RgbColor(0, 0, 32)
  };

  for (const auto& color : testColors) {
    for (int i = 0; i < LED_COUNT; i++) {
      strip.SetPixelColor(i, color);
    }
    strip.Show();
    delay(2000);
  }

  // Return LEDs to off state after test
  for (int i = 0; i < LED_COUNT; i++) {
    strip.SetPixelColor(i, RgbColor(0, 0, 0));
  }
  strip.Show();
  
  ESP_LOGI(TAG, "LEDs initialized");
}

void LEDController::displayTrainPositions() {
  // TODO: Implement LED display logic based on train positions
  // For now, just show a simple pattern to indicate the system is working
  static uint8_t animationHue = 0;

  for (int i = 0; i < LED_COUNT; i++) {
    strip.SetPixelColor(i, HslColor(animationHue / 255.0f, 1.0f, 0.1f));
  }

  strip.Show();
  animationHue++;
}
