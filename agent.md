# LinkLight Project - AI Agent Guide

## Project Overview

LinkLight is an ESP32-S3 firmware project that displays real-time SoundTransit Link light rail train positions on a WS2812 LED strip. The system fetches train data from the OneBusAway API and visualizes train locations on a custom PCB with LED indicators.

### Key Technologies

- **Hardware**: ESP32-S3 microcontroller (DevKitC-1), WS2812 LED strip (50 LEDs)
- **Framework**: Arduino-ESP32
- **Build System**: PlatformIO
- **Libraries**: WiFiManager, NeoPixelBus, ArduinoJson, Preferences

## Project Architecture

### Boot Flow

1. **LED Initialization** - Visual feedback that system is starting
2. **Load Configuration** - Retrieve saved settings from NVS (Non-Volatile Storage)
3. **WiFi Setup** - Connect using WiFiManager (captive portal on first boot)
4. **Web Server Start** - Configuration interface on port 80
5. **Main Loop** - API polling (every 30s) + LED updates

### Core Components

#### WiFi Provisioning (WiFiManager)

- Creates "LinkLight-Setup" AP on first boot or connection failure
- 180-second timeout for captive portal
- Credentials saved automatically for future boots

#### Web Server (WebServer)

- `GET /` - Status page showing IP, home station
- `GET /config` - Configuration form
- `POST /config` - Save settings (home station, API key, route ID)

#### Persistent Storage (Preferences)

- Namespace: `linklight`
- Keys: `homeStation`, `apiKey`, `routeId`
- Uses ESP32 NVS (Non-Volatile Storage)

#### LED Control (NeoPixelBus)

- 50 WS2812 LEDs on GPIO 8
- Currently shows animated pattern (train logic TODO)
- Uses HSL color space for smooth animations

#### API Integration

- Endpoint: `https://api.pugetsound.onebusaway.org/api/where/trips-for-route/{routeId}.json?key={apiKey}`
- Default route: `40_102574` (Link Light Rail)
- Polling interval: 30 seconds
- Response parsed with ArduinoJson

## File Structure

```
LinkLight/
├── src/
│   └── main.cpp              # Main application (281 lines)
│       ├── setup()           # Initialization sequence
│       ├── loop()            # Main event loop
│       ├── setupWiFi()       # WiFiManager initialization
│       ├── setupWebServer()  # Web server routes
│       ├── setupLEDs()       # NeoPixelBus initialization
│       ├── loadPreferences() # Load config from NVS
│       ├── savePreferences() # Save config to NVS
│       ├── handleRoot()      # GET / handler
│       ├── handleConfig()    # GET /config handler
│       ├── handleSaveConfig() # POST /config handler
│       ├── updateTrainPositions() # API polling (TODO: parse response)
│       └── displayTrainPositions() # LED update logic (TODO: show trains)
│
├── include/
│   └── config.h              # Configuration constants
│       ├── WiFi settings (timeout, portal name)
│       ├── LED settings (pin, count)
│       ├── Web server settings (port)
│       ├── API settings (base URL, update interval)
│       ├── Preference keys (namespace, key names)
│       └── Default values (route ID)
│
├── .github/workflows/
│   ├── pr-build.yml         # CI pipeline for pull requests
│   │   ├── Build job (on pull request events)
│   │   └── Artifacts: firmware.bin, firmware.elf (as zip)
│   └── build-release.yml    # CI/CD pipeline for releases
│       ├── Build job (on release creation and workflow_dispatch)
│       ├── Release job (on release creation only)
│       └── Artifacts: firmware.bin, firmware.elf
│
├── platformio.ini            # PlatformIO configuration
│   ├── Target: esp32-s3-devkitc-1
│   ├── Platform: espressif32
│   ├── Framework: arduino
│   └── Dependencies: WiFiManager 2.0.17, NeoPixelBus 2.8.4, ArduinoJson 7.2.0
│
├── README.md                 # User-facing documentation
├── LICENSE                   # MIT License
└── agent.md                  # This file
```

## Development Workflow

### Building the Project

#### Prerequisites

- Python 3.x
- PlatformIO Core (`pip install platformio`)

#### Commands

```bash
# Build firmware
platformio run

# Build and upload
platformio run --target upload

# Open serial monitor
platformio device monitor -b 115200

# Clean build
platformio run --target clean
```

**Note**: Building requires network access to these domains:

- `api.registry.platformio.org` - PlatformIO registry
- `github.com` / `raw.githubusercontent.com` - Library downloads
- `dl.espressif.com` - ESP32 toolchain

### Testing on Hardware

1. **Upload firmware**: `platformio run --target upload`
2. **Connect to WiFi**:
   - Look for "LinkLight-Setup" AP
   - Connect and configure WiFi credentials
3. **Find IP address**: Check serial monitor at 115200 baud
4. **Access web interface**: Navigate to `http://<IP_ADDRESS>`
5. **Configure settings**: Enter home station, API key, route ID

### Creating a Release

1. Go to the GitHub repository page
2. Click on "Releases" in the right sidebar
3. Click "Create a new release"
4. Create a new tag (e.g., v1.0.0) and fill in release details
5. Click "Publish release"

GitHub Actions will automatically:
1. Build firmware.bin and firmware.elf
2. Attach binaries to the release

## Common Tasks for AI Agents

### Adding a New Configuration Parameter

1. **Add constant to `include/config.h`**:

   ```cpp
   #define PREF_NEW_PARAM "newParam"
   ```

2. **Add global variable to `src/main.cpp`**:

   ```cpp
   String newParam = "";
   ```

3. **Update `loadPreferences()`**:

   ```cpp
   newParam = preferences.getString(PREF_NEW_PARAM, "default");
   ```

4. **Update `savePreferences()`**:

   ```cpp
   preferences.putString(PREF_NEW_PARAM, newParam);
   ```

5. **Add to web interface** in `handleConfig()` and `handleSaveConfig()`

### Changing LED Behavior

**Location**: `displayTrainPositions()` function in `src/main.cpp`

**Current behavior**: Animated rainbow pattern (placeholder)

**To implement train display**:

1. Parse train positions in `updateTrainPositions()`
2. Store train data in global variables/structs
3. Map train positions to LED indices in `displayTrainPositions()`
4. Set appropriate colors (e.g., green for northbound, red for southbound)

### Adding a New Web Endpoint

1. **Create handler function**:

   ```cpp
   void handleNewEndpoint() {
     String html = "...";
     server.send(200, "text/html", html);
   }
   ```

2. **Register in `setupWebServer()`**:

   ```cpp
   server.on("/new", HTTP_GET, handleNewEndpoint);
   ```

### Modifying API Polling

**Location**: `updateTrainPositions()` function in `src/main.cpp`

**Current implementation**:

- Fetches data from OneBusAway API
- Parses JSON with ArduinoJson
- TODO: Extract train position data

**API Response Structure** (needs investigation):

- Check OneBusAway API documentation
- Use serial output to debug response structure
- Update JsonDocument parsing accordingly

### Adding New Library Dependencies

1. **Update `platformio.ini`**:

   ```ini
   lib_deps =
       existing/Library @ ^1.0.0
       new/Library @ ^2.0.0
   ```

2. **Include in source files**:

   ```cpp
   #include <NewLibrary.h>
   ```

3. **PlatformIO auto-downloads on next build**

## Important Considerations

### Memory Management

- ESP32-S3 has PSRAM enabled (`-DBOARD_HAS_PSRAM`)
- NeoPixelBus requires continuous memory for pixel data
- ArduinoJson uses dynamic allocation - size JsonDocument appropriately
- Preferences (NVS) has limited write cycles - minimize saves

### WiFi Stability

- WiFiManager has 180s timeout (adjustable in `config.h`)
- On connection failure, device restarts and recreates AP
- Web server only accessible when WiFi connected
- Consider adding WiFi reconnection logic to `loop()`

### LED Performance

- 50 LEDs @ 60mA each = up to 3A current draw
- Ensure adequate power supply (5V, 3A+ recommended)
- Use level shifter if needed (ESP32 3.3V → WS2812 5V data)
- `strip.Show()` blocks briefly - avoid calling too frequently

### API Rate Limits

- Current polling: every 30 seconds
- OneBusAway may have rate limits - check API terms
- API key required (get from pugetsound.onebusaway.org)
- Error handling exists but could be enhanced

### Security

- API key stored in plaintext in NVS
- Web interface has no authentication
- Suitable for home network, not public exposure
- HTTPS used for API requests (certificate validation handled by ESP32)

## Known TODOs and Future Enhancements

### Immediate TODOs (marked in code)

1. **`updateTrainPositions()`**: Parse train position data from API response
2. **`displayTrainPositions()`**: Map train positions to LED strip

### Potential Enhancements

- WiFi reconnection without restart
- Multiple route support
- Home station highlighting
- Train direction indicators (color coding)
- Web interface improvements (live updates, better styling)
- OTA (Over-The-Air) firmware updates
- AP mode web server (configure without WiFi)
- Train schedule integration
- Alert notifications
- Power-saving modes
- Brightness control
- Multiple LED strip support

## Debugging Tips

### Serial Monitor

- Baud rate: 115200
- Shows startup sequence, WiFi status, API requests
- Debug level: 4 (verbose) - set in `platformio.ini`

### Common Issues

**Build Fails**:

- Check PlatformIO registry access (network whitelist)
- Verify library versions in `platformio.ini`
- Clean and rebuild: `platformio run --target clean`

**WiFi Won't Connect**:

- Check serial monitor for error messages
- Verify "LinkLight-Setup" AP appears
- Try power cycle (hard reset)
- Check WiFiManager timeout (180s default)

**LEDs Not Working**:

- Verify GPIO 8 connection
- Check power supply (5V, adequate current)
- Try changing `LED_PIN` in `config.h`
- Test with simple pattern first

**API Returns Errors**:

- Verify API key is valid
- Check route ID is correct
- Test URL manually in browser
- Check serial monitor for HTTP error codes

## Version Control

**Main Branch**: `main` (production-ready code)
**Feature Branches**: `copilot/*` (agent-created features)

**Commit Message Format**: Descriptive, imperative mood

- Good: "Add WiFi reconnection logic"
- Bad: "Fixed stuff"

**Pull Requests**:

- Include description of changes
- Reference any related issues
- Ensure builds pass in CI

## Additional Resources

- **PlatformIO Docs**: https://docs.platformio.org/
- **ESP32 Arduino Core**: https://github.com/espressif/arduino-esp32
- **WiFiManager**: https://github.com/tzapu/WiFiManager
- **NeoPixelBus**: https://github.com/Makuna/NeoPixelBus
- **ArduinoJson**: https://arduinojson.org/
- **OneBusAway API**: https://developer.onebusaway.org/
- **SoundTransit API**: https://api.pugetsound.onebusaway.org/

## Quick Reference

**Hardware Specs**:

- MCU: ESP32-S3 (dual-core, 240MHz, WiFi + BLE)
- LED Strip: WS2812/NeoPixel (50 LEDs, GPIO 8)
- Power: 5V (USB or external, 3A+ recommended)

**Network Info**:

- AP Name: `LinkLight-Setup`
- Web Interface: `http://<device-ip>`
- API: `api.pugetsound.onebusaway.org`

**Key Constants** (in `config.h`):

- `LED_PIN`: GPIO 8
- `LED_COUNT`: 50
- `WEB_SERVER_PORT`: 80
- `API_UPDATE_INTERVAL`: 30000ms (30s)
- `WIFI_PORTAL_TIMEOUT`: 180s

This guide should help you understand the project structure and make effective contributions!
