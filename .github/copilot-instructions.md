# LinkLight - Copilot Instructions

## Project Overview

LinkLight is an ESP32-S3 embedded firmware project that displays real-time
SoundTransit Link light rail train positions on a WS2812 LED strip. The system
fetches train data from the OneBusAway API and visualizes train locations using
a custom PCB with 50 LED indicators.

**Technology Stack:**

- Hardware: ESP32-S3 microcontroller (seeed_xiao_esp32s3), WS2815 LED strip
- Framework: Arduino-ESP32
- Build System: PlatformIO
- Languages: C++ (Arduino)
- Key Libraries: WiFiManager 2.0.17, NeoPixelBus 2.8.4, ArduinoJson 7.2.0,
  Preferences (built-in)

## Building and Testing

### Prerequisites

- Python 3.x (tested with 3.12.3)
- PlatformIO Core: Install with `pip install platformio`

### Build Commands

**ALWAYS run these commands from the repository root directory.**

```bash
# Build firmware (initial build downloads dependencies and may take a few minutes)
pio run

# Clean build artifacts before rebuilding
pio run --target clean

# Build for specific environment (if multiple environments exist)
pio run --environment seeed_xiao_esp32s3

# Monitor serial output (requires connected device)
pio device monitor -b 115200
```

**Build Success Indicators:**

- Exit code 0
- Message: `[SUCCESS] Took XX.XX seconds`
- Firmware files created: `.pio/build/seeed_xiao_esp32s3/firmware.bin`
  and `firmware.elf`

**Network Requirements:**

The build process requires internet access to:

- `api.registry.platformio.org` - PlatformIO registry
- `github.com` / `raw.githubusercontent.com` - Library downloads
- `dl.espressif.com` - ESP32 toolchain

### Testing

**No automated tests currently exist in this project.** Testing is performed
manually on hardware:

1. Build and upload firmware: `pio run --target upload`
2. Connect to "LinkLight-Setup" WiFi AP and configure network
3. Access web interface at device IP address
4. Verify LED strip displays expected animations
5. Check serial monitor at 115200 baud for debug output

## Project Structure

### Key Files and Directories

```text
LinkLight/
├── src/main.cpp              # Main application code (281 lines)
│                             # Contains setup(), loop(), and all core functions
├── include/config.h          # Configuration constants (pins, timeouts, URLs)
├── data/                     # HTML files for web interface (served via LittleFS)
│   ├── index.html            # Status page
│   ├── config.html           # Configuration form
│   └── config_saved.html     # Save confirmation page
├── platformio.ini            # PlatformIO build configuration
│                             # Defines target (seeed_xiao_esp32s3), dependencies
├── .github/workflows/        # CI/CD pipelines
│   ├── pr-build.yml          # Builds on pull requests
│   ├── build-release.yml     # Builds and attaches binaries to releases
│   └── pr-label-required.yml # Ensures PRs have required labels
├── README.md                 # User-facing documentation
└── .markdownlint-cli2.jsonc  # Markdown linting configuration
```

### Core Architecture

**Boot Sequence:**

1. LED initialization → 2. Load config from NVS → 3. WiFi setup
   (captive portal) → 4. Start web server → 5. Main loop
   (API polling + LED updates)

**Main Components:**

- **WiFi**: WiFiManager creates "LinkLight-Setup" AP on first boot
  (180s timeout)
- **Web Server**: Runs on port 80, provides `/` (status), `/config`
  (settings form), and POST handler
- **Storage**: Preferences library stores apiKey, hostname
  in ESP32 NVS
- **LED Control**: NeoPixelBus controls 50 WS2812 LEDs on GPIO 8
- **API Client**: Polls OneBusAway API every 30s for train positions

**Key Functions in src/main.cpp:**

- `setup()` - Initialization sequence
- `loop()` - Main event loop
- `setupWiFi()` - WiFiManager setup
- `setupWebServer()` - Route registration
- `updateTrainPositions()` - API polling (TODO: parse response)
- `displayTrainPositions()` - LED control (TODO: show actual trains)

## Common Development Tasks

### Adding Configuration Parameters

1. Add constant to `include/config.h`:
   `#define PREF_NEW_PARAM "newParam"`
2. Add global variable in `src/main.cpp`: `String newParam = "";`
3. Update `loadPreferences()` to read from NVS:
   `newParam = preferences.getString(PREF_NEW_PARAM, "default");`
4. Update `savePreferences()` to write to NVS:
   `preferences.putString(PREF_NEW_PARAM, newParam);`
5. Add form field to web interface in `handleConfig()` and
   `handleSaveConfig()`

### Modifying LED Behavior

Edit `displayTrainPositions()` function in `src/main.cpp`. Currently shows
animated rainbow pattern. To implement train display, parse positions in
`updateTrainPositions()` and map to LED indices.

### Adding Dependencies

Update `lib_deps` section in `platformio.ini`. PlatformIO auto-downloads on
next build:

```ini
lib_deps =
    existing/Library @ ^1.0.0
    new/Library @ ^2.0.0
```

## Pull Request Requirements

**All PRs must have at least one label:**

- `enhancement` - New features and improvements
- `bug` - Bug fixes
- `development` - Build process/workflow/tooling changes
- `dependencies` - Dependency updates
- `documentation` - Documentation updates

**CI Checks:**

- Build verification runs on all PRs
  (see `.github/workflows/pr-build.yml`)
- Label requirement enforced
  (see `.github/workflows/pr-label-required.yml`)
- Markdown linting with markdownlint-cli2

## Important Constraints

### Design Requirements

- Do not use emojis anywhere in the web interface or any user-facing content.

### Memory Management

- ESP32-S3 has PSRAM enabled (`-DBOARD_HAS_PSRAM`)
- NeoPixelBus requires continuous memory for 50 LEDs
- ArduinoJson uses dynamic allocation - size JsonDocument appropriately
- NVS has limited write cycles - minimize preference saves

### API and Networking

- API endpoint:
  `https://api.pugetsound.onebusaway.org/api/where/trips-for-route/{routeId}.json`
- Default route: `40_102574` (Link Light Rail)
- Polling interval: 30 seconds
- API key required (stored in NVS, no encryption)

### Hardware Specifics

- 50 WS2812 LEDs on GPIO 8
- LED current draw: up to 3A at full brightness
  (ensure adequate 5V supply)
- Serial monitor baud rate: 115200
- Web server has no authentication (suitable for home networks only)

## Trust These Instructions

The build commands, file locations, and architectural details documented here
have been validated. When working on this codebase, trust these instructions
and only search for additional information if something is unclear or appears
incorrect.
