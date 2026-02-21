# LinkLight

LinkLight is a project that displays real-time
[Sound Transit Link Light Rail](https://www.soundtransit.org/ride-with-us/routes-schedules/link-light-rail)
train positions on a WS2815 LED strip.

## What it does

LinkLight uses the [OneBusAway API](https://onebusaway.org/) to retrieve live train positions
for the Seattle area Link Light Rail system. Each LED on the strip corresponds to position on the
1 and 2 line, and lights up based on train presence along the line.

- Line 1 trains are shown in green (configurable)
- Line 2 trains are shown in blue (configurable)
- When trains from both lines share a section, the LED lights up in yellow (configurable)

A built-in web interface lets you view real-time train status, check system
logs, and adjust settings from any device on your network.

## Getting started

### Prerequisites

- SeeedStudio XIAO ESP32-S3 microcontroller ([Amazon](https://www.amazon.com/dp/B0BYSB66S5))
- WS2815 LED strip (160 LEDs) ([Amazon](https://www.amazon.com/dp/B07LG6J39V))
- A [SoundTransit OneBusAway API key](https://www.soundtransit.org/help-contacts/business-information/open-transit-data-otd)

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
4. **Open the web interface** — navigate to `http://linklight` in your browser.
5. **Enter your API key** — go to the **Configuration** page and paste in your
   SoundTransit OneBusAway API key. Adjust any other settings and save.

### Web interface

| Page                                        | Description                                   |
| ------------------------------------------- | --------------------------------------------- |
| [Status](http://linklight/)                 | Shows device hostname and IP address          |
| [Configuration](http://linklight/config/)   | Configure API key, colors, timezone, hostname |
| [Train status](http://linklight/trains/)    | Real-time train positions and LED state       |
| [System logs](http://linklight/logs/)       | Recent log output from the device             |
| [Firmware update](http://linklight/update/) | Flash firmware or filesystem over the browser |

### Configuration options

| Setting              | Description                                                 | Default            |
| -------------------- | ----------------------------------------------------------- | ------------------ |
| Hostname             | mDNS hostname for the device                                | `LinkLight`        |
| API key              | OneBusAway API key                                          | _(none)_           |
| Timezone             | Timezone for log timestamps                                 | Pacific Time       |
| Refresh interval     | How often to poll the API (seconds)                         | `30`               |
| At-station threshold | Seconds before arrival a train is considered at the station | `10`               |
| Line 1 color         | LED color for Link 1 trains                                 | SoundTransit Green |
| Line 2 color         | LED color for Link 2 trains                                 | SoundTransit Blue  |
| Overlap color        | LED color when both lines share a station                   | Yellow             |

## Technical overview

### Hardware

| Component       | Details                          |
| --------------- | -------------------------------- |
| Microcontroller | SeeedStudio XIAO ESP32-S3        |
| LED strip       | WS2815, 160 LEDs, data on GPIO 8 |

### Software stack

| Component          | Library / technology   |
| ------------------ | ---------------------- |
| Build system       | PlatformIO             |
| Framework          | Arduino (ESP32)        |
| WiFi provisioning  | tzapu/WiFiManager      |
| LED control        | makuna/NeoPixelBus     |
| JSON parsing       | bblanchon/ArduinoJson  |
| HTML templating    | floatplane/Ministache  |
| WebSocket server   | links2004/WebSockets   |
| Persistent storage | Preferences (built-in) |
| Filesystem         | LittleFS (built-in)    |

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
│   ├── logs.html             # System logs
│   └── update.html           # Firmware/filesystem update
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

# Upload firmware over the air (set upload_port to the device IP in platformio.ini)
pio run --target upload --upload-protocol espota

# Upload filesystem over the air
pio run --target uploadfs
```

The first build downloads all dependencies and the ESP32 toolchain, which may
take several minutes.

### CI/CD

GitHub Actions workflows are provided for automated builds:

- **`pr-build.yml`** — builds firmware and filesystem on every pull request and
  uploads the binaries as a build artifact.
- **`build-release.yml`** — builds and attaches `firmware.bin`, `firmware.elf`,
  and `littlefs.bin` to every GitHub release.
