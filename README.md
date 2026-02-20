# LinkLight

LinkLight is an ESP32-S3 firmware project that displays real-time
[Sound Transit Link Light Rail](https://www.soundtransit.org/ride-with-us/routes-schedules/link-light-rail)
train positions on a custom PCB with a WS2815 LED strip.

## What it does

LinkLight connects to your home WiFi network and continuously polls the
[OneBusAway API](https://onebusaway.org/) to retrieve live train positions for
the Seattle area Link Light Rail system. Each LED on the strip corresponds to a
station, and lights up when a train is at or approaching that station.

- Line 1 trains are shown in green (configurable)
- Line 2 trains are shown in blue (configurable)
- When both lines share a station, the LED lights up in yellow (configurable)

A built-in web interface lets you view real-time train status, check system
logs, and adjust settings from any device on your network.

## Getting started

### Prerequisites

- SeeedStudio XIAO ESP32-S3 microcontroller
- Compatible WS2815 LED strip (160 LEDs)
- An [OneBusAway API key](https://onebusaway.puget-sound-api.org/p/individual.action)

### First-time setup

1. **Flash the firmware** — see [Building from source](#building-from-source)
   for instructions, or download a pre-built release from the
   [Releases](../../releases) page and flash using
   [esptool](https://docs.espressif.com/projects/esptool/en/latest/).
2. **Connect to the setup portal** — on first boot, LinkLight broadcasts a WiFi
   access point named `LinkLight-Setup`. Connect to it from your phone or
   computer. A captive portal will open automatically; if it does not, navigate
   to `192.168.4.1` in your browser.
3. **Configure WiFi** — enter your home network credentials in the captive
   portal and save. LinkLight will restart and connect to your network.
4. **Open the web interface** — navigate to the device's IP address (shown on
   the serial monitor at 115200 baud, or found via your router's DHCP table).
5. **Enter your API key** — go to the **Configuration** page and paste in your
   OneBusAway API key. Adjust any other settings and save.

### Web interface

| Page | URL | Description |
| --- | --- | --- |
| Status | `/` | Shows device hostname and IP address |
| Configuration | `/config` | Configure API key, colors, timezone, hostname |
| Train status | `/trains` | Real-time train positions and LED state |
| System logs | `/logs` | Recent log output from the device |

### Configuration options

| Setting | Description | Default |
| --- | --- | --- |
| Hostname | mDNS hostname for the device | `LinkLight` |
| API key | OneBusAway API key | _(none)_ |
| Timezone | Timezone for log timestamps | Pacific Time |
| Refresh interval | How often to poll the API (seconds) | `30` |
| Line 1 color | LED color for Link 1 trains | Green |
| Line 2 color | LED color for Link 2 trains | Blue |
| Overlap color | LED color when both lines share a station | Yellow |

## Technical overview

### Hardware

| Component | Details |
| --- | --- |
| Microcontroller | SeeedStudio XIAO ESP32-S3 |
| LED strip | WS2815, 160 LEDs, data on GPIO 8 |

### Software stack

| Component | Library / technology | Version |
| --- | --- | --- |
| Build system | PlatformIO | — |
| Framework | Arduino (ESP32) | — |
| WiFi provisioning | tzapu/WiFiManager | 2.0.17 |
| LED control | makuna/NeoPixelBus | 2.8.x |
| JSON parsing | bblanchon/ArduinoJson | 7.2.0 |
| HTML templating | floatplane/Ministache | 1.0.0 |
| WebSocket server | links2004/WebSockets | 2.6.1 |
| Persistent storage | Preferences (built-in) | — |
| Filesystem | LittleFS (built-in) | — |

### Architecture

The firmware runs two FreeRTOS tasks:

- **Core 1 (loop task)** — handles OTA updates, serves web requests, and
  updates the LED strip and WebSocket clients whenever new data is ready.
- **Core 0 (TrainUpdate task)** — polls the OneBusAway API on a configurable
  interval and notifies the loop task when fresh train data is available.

Train positions are fetched from the OneBusAway `/trips-for-route` endpoint for
Line 1 (`40_100479`) and Line 2 (`40_2LINE`). Each train's closest and next
stop is mapped to a physical LED index using a hardcoded station-to-LED table.

The web interface is served from LittleFS using Ministache templates. Live
train and LED state updates are pushed to the browser over WebSocket on
port 81.

### Project layout

```text
LinkLight/
├── src/main.cpp              # Entry point; setup() and loop()
├── include/
│   ├── config.h              # Pin assignments, timeouts, default values
│   ├── LEDController.h       # LED strip management
│   ├── TrainDataManager.h    # API polling and train data model
│   ├── WebServerManager.h    # HTTP and WebSocket server
│   ├── PreferencesManager.h  # NVS read/write
│   ├── StopData.h            # Stop ID to station name map
│   └── ...                   # Other component headers
├── data/                     # LittleFS web pages (HTML templates)
│   ├── index.html            # Status page
│   ├── config.html           # Configuration form
│   ├── trains.html           # Live train status
│   └── logs.html             # System logs
├── sample/                   # Sample OneBusAway API responses
├── platformio.ini            # PlatformIO build configuration
└── .github/workflows/        # CI/CD pipelines
```

## Building from source

### Build prerequisites

- Python 3.x
- PlatformIO Core (`pip install platformio`)

### Build commands

```bash
# Build firmware and filesystem
pio run
pio run --target buildfs

# Upload firmware over USB
pio run --target upload

# Upload firmware over the air (set upload_port to the device IP in platformio.ini)
pio run --target upload --upload-protocol espota

# Open serial monitor at 115200 baud
pio device monitor -b 115200

# Clean build artifacts
pio run --target clean
```

The first build downloads all dependencies and the ESP32 toolchain, which may
take several minutes. Subsequent builds are faster thanks to PlatformIO's
dependency cache.

### CI/CD

GitHub Actions workflows are provided for automated builds:

- **`pr-build.yml`** — builds firmware and filesystem on every pull request and
  uploads the binaries as a build artifact.
- **`build-release.yml`** — builds and attaches `firmware.bin`, `firmware.elf`,
  and `littlefs.bin` to every GitHub release.
