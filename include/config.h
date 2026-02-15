#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_PORTAL_TIMEOUT 180  // Timeout for WiFi portal in seconds

// LED Configuration
#define LED_PIN 8                 // GPIO pin for NeoPixel data
#define LED_COUNT 50              // Number of LEDs in the strip

// Web Server Configuration
#define WEB_SERVER_PORT 80

// API Configuration
#define API_BASE_URL "https://api.pugetsound.onebusaway.org/api/where"
#define API_KEY_PARAM "key"       // API key parameter name
#define API_UPDATE_INTERVAL (30 * 1000) // Update interval in milliseconds (30 seconds)

// Preferences Keys
#define PREF_NAMESPACE "linklight"
#define PREF_HOME_STATION "homeStation"
#define PREF_API_KEY "apiKey"
#define PREF_ROUTE_ID "routeId"

// Default values
#define DEFAULT_ROUTE_ID "40_102574"  // Link Light Rail route ID

#endif // CONFIG_H
