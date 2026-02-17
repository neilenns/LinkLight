#ifndef CONFIG_H
#define CONFIG_H

// OTA Configuration
#define OTA_HOSTNAME "LinkLight"
#define OTA_PASSWORD ""  // Empty by default, can be set for security

// WiFi Configuration
#define WIFI_PORTAL_TIMEOUT 180  // Timeout for WiFi portal in seconds

// LED Configuration
#define LED_PIN 8                 // GPIO pin for NeoPixel data
#define LED_COUNT 55              // Number of LEDs in the strip

// Web Server Configuration
#define WEB_SERVER_PORT 80

// API Configuration
#define API_BASE_URL "https://api.pugetsound.onebusaway.org/api/where"
#define API_KEY_PARAM "key"       // API key parameter name
#define API_UPDATE_INTERVAL (30 * 1000) // Update interval in milliseconds (30 seconds)

// Route IDs
#define LINE_1_ROUTE_ID "40_100479"  // Link Light Rail Line 1
#define LINE_2_ROUTE_ID "40_2LINE"   // Link Light Rail Line 2
#define LINE_1_NAME "Line 1"
#define LINE_2_NAME "Line 2"

// Train State Threshold
#define MIN_DEPARTED_SECONDS 30  // Minimum seconds after departure to consider train as "moving"

// Preferences Keys
#define PREF_NAMESPACE "linklight"
#define PREF_HOME_STATION "homeStation"
#define PREF_API_KEY "apiKey"
#define PREF_HOSTNAME "hostname"

// Default Values
#define DEFAULT_HOSTNAME "LinkLight"

#endif // CONFIG_H
