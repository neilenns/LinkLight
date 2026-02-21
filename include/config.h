#ifndef CONFIG_H
#define CONFIG_H

// OTA Configuration
#define OTA_HOSTNAME "LinkLight"
#define OTA_PASSWORD ""  // Empty by default, can be set for security

// WiFi Configuration
#define WIFI_PORTAL_TIMEOUT 180  // Timeout for WiFi portal in seconds

// LED Configuration
#define LED_PIN 8                  // GPIO pin for NeoPixel data
#define LED_COUNT 160              // Number of LEDs in the strip

// Web Server Configuration
#define WEB_SERVER_PORT 80
#define WEB_SOCKET_PORT 81

// API Configuration
#define API_BASE_URL "https://api.pugetsound.onebusaway.org/api/where"
#define API_KEY_PARAM "key"       // API key parameter name
#define API_UPDATE_INTERVAL (30 * 1000) // Update interval in milliseconds (30 seconds)

// Route IDs
#define LINE_1_ROUTE_ID "40_100479"  // Link Light Rail Line 1
#define LINE_2_ROUTE_ID "40_2LINE"   // Link Light Rail Line 2

// Distance a train should be within to be considered at the station
#define AT_STATION_THRESHOLD 10

// Preferences Keys
#define PREF_NAMESPACE "linklight"
#define PREF_API_KEY "apiKey"
#define PREF_HOSTNAME "hostname"
#define PREF_TIMEZONE "timezone"
#define PREF_UPDATE_INTERVAL "updateInterval"
#define PREF_AT_STATION_THRESHOLD "atStationThresh"
#define PREF_LINE1_COLOR "line1Color"
#define PREF_LINE2_COLOR "line2Color"
#define PREF_SHARED_COLOR "sharedColor"
#define MAX_PREFERENCE_LENGTH 64  // Maximum length for preference strings

// Default Values
#define DEFAULT_HOSTNAME "LinkLight"
#define DEFAULT_TIMEZONE "PST8PDT,M3.2.0,M11.1.0"  // Pacific Time with DST
#define DEFAULT_UPDATE_INTERVAL 30  // Default update interval in seconds
#define DEFAULT_AT_STATION_THRESHOLD 10  // Default at-station threshold in seconds
#define DEFAULT_LINE1_COLOR "#28813F"  // Official SoundTransit green for Line 1
#define DEFAULT_LINE2_COLOR "#007CAD"  // Official SoundTransit blue for Line 2
#define DEFAULT_SHARED_COLOR "#232300"  // Yellow for shared/overlap

#endif // CONFIG_H
