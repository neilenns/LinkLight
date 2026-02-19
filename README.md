# LinkLight

Custom PCB and ESP32 code to display Link Light Rail train positions on WS2812 LEDs.

## Overview

LinkLight is a project to display the current position of SoundTransit Link light rail trains on a custom PCB using an ESP32-S3 microcontroller and WS2812 LEDs.

## Features

- **WiFi Provisioning**: Easy WiFi setup using WiFiManager with captive portal
- **OTA Updates**: Over-the-air firmware updates via WiFi (no USB cable needed)
- **Web-based Firmware Updates**: Upload firmware and filesystem updates directly from the web interface
- **LED Control**: WS2812 LED strip control using NeoPixelBus library
- **Web Configuration**: Simple web interface to configure home station and API settings
- **Persistent Storage**: Configuration saved to ESP32 flash memory
- **Real-time Updates**: Fetches train position data from SoundTransit Open Transit Data API
- **GitHub Workflow Automation**: Automated builds and releases

## Hardware Requirements

- ESP32-S3 development board (e.g., ESP32-S3-DevKitC-1)
- WS2812 LED strip (up to 50 LEDs)
- 5V power supply (adequate for LED strip)
- Level shifter (if needed for LED data line)

## Software Stack

- **Framework**: Arduino-ESP32
- **Build System**: PlatformIO
- **IDE**: Visual Studio Code with PlatformIO extension
- **Libraries**:
  - WiFiManager (^2.0.17) - WiFi provisioning
  - NeoPixelBus (^2.8.0) - LED control
  - ArduinoJson (^7.2.0) - JSON parsing
  - Preferences (built-in) - Persistent storage

## Getting Started

### Prerequisites

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)

### Building the Project

1. Clone this repository:
   ```bash
   git clone https://github.com/neilenns/LinkLight.git
   cd LinkLight
   ```

2. Open the project in Visual Studio Code

3. Build the project:
   - Click the PlatformIO icon in the left sidebar
   - Click "Build" or press `Ctrl+Alt+B`

### Uploading to ESP32

#### Initial Upload (via USB)

1. Connect your ESP32-S3 board to your computer via USB

2. Upload the firmware:
   - Click "Upload" in PlatformIO or press `Ctrl+Alt+U`

3. Upload the filesystem (required for web pages):
   - In PlatformIO, open the Project Tasks menu
   - Under "Platform", click "Upload Filesystem Image"
   - Alternatively, run: `pio run --target uploadfs`
   - This uploads the HTML files from the `data` folder to the ESP32's LittleFS filesystem

#### OTA Upload (Over-The-Air)

After the initial upload, you can update the firmware wirelessly using either:

**Method 1: PlatformIO OTA (for developers)**

1. Make sure your computer is on the same network as the ESP32

2. Find your ESP32's IP address (displayed in serial monitor on boot or visible in your router)

3. Edit `platformio.ini` and uncomment/update the OTA settings:
   ```ini
   upload_protocol = espota
   upload_port = 192.168.1.100  ; Replace with your ESP32's IP address
   upload_flags = 
       --port=3232
   ```

4. Upload the firmware:
   - Click "Upload" in PlatformIO or press `Ctrl+Alt+U`
   - The firmware will be uploaded over WiFi instead of USB

**Method 2: Web-based OTA (recommended for end users)**

1. Navigate to your ESP32's IP address in a web browser

2. Click "Firmware Update" from the home page

3. Upload new firmware:
   - Click "Select Firmware File" and choose the `firmware.bin` file
   - Click "Upload Firmware" and wait for the upload to complete
   - The device will automatically reboot with the new firmware

4. Upload new filesystem (if needed):
   - Click "Select Filesystem File" and choose the `littlefs.bin` file
   - Click "Upload Filesystem" and wait for the upload to complete
   - The device will automatically reboot with the new filesystem

**Note:** 
- With web-based OTA, both firmware and filesystem can be updated without a USB connection
- Firmware files can be obtained from GitHub releases or built locally with PlatformIO
- Make sure to use the correct `.bin` files - `firmware.bin` for firmware updates and `littlefs.bin` for filesystem updates

### First-Time Setup

1. After uploading, the ESP32 will create a WiFi access point named `LinkLight-Setup`

2. Connect to this access point with your phone or computer

3. Configure your WiFi credentials in the captive portal

4. Once connected, find the ESP32's IP address in the serial monitor

5. Open a web browser and navigate to the ESP32's IP address

6. Click "Configuration" and enter:
   - Your home station name
   - OneBusAway API key (get one from [OneBusAway](https://pugetsound.onebusaway.org/))
   - Route ID (default is for Link Light Rail)

## Configuration

### LED Configuration

Edit `include/config.h` to change:
- LED pin (default: GPIO 8)
- Number of LEDs (default: 50)

### API Configuration

The default configuration uses:
- API URL: `https://api.pugetsound.onebusaway.org/api/where`
- Route ID: `40_102574` (Link Light Rail)
- Update interval: 30 seconds

## Development

### Project Structure

```
LinkLight/
├── data/               # Static HTML files for web interface
│   ├── index.html      # Main page
│   ├── config.html     # Configuration page
│   ├── config_saved.html  # Configuration saved confirmation
│   ├── logs.html       # System logs page
│   └── update.html     # Firmware/filesystem update page
├── include/            # Header files
│   └── config.h        # Configuration constants
├── src/                # Source code
│   └── main.cpp        # Main application
└── platformio.ini      # PlatformIO configuration
```

### Building Locally

```bash
# Build the project
pio run

# Upload to device via USB
pio run --target upload

# Upload to device via OTA (after configuring platformio.ini with ESP32 IP)
pio run --target upload

# Upload filesystem (HTML files)
pio run --target uploadfs

# Monitor serial output
pio device monitor

# Build and upload and monitor
pio run --target upload && pio device monitor
```

### OTA Updates

The device hostname for OTA is set to `LinkLight` by default. You can find the device on your network using:
- mDNS: `LinkLight.local`
- IP address shown in serial monitor on boot
- Check your router's DHCP client list

To enable password protection for OTA updates, edit `include/config.h` and set `OTA_PASSWORD` to a non-empty string.

#### Obtaining Firmware Files for Web-based Updates

**From GitHub Releases:**
1. Go to the [Releases page](https://github.com/neilenns/LinkLight/releases)
2. Download `firmware.bin` for firmware updates
3. Download `littlefs.bin` for filesystem updates (if available)

**Build Locally:**
```bash
# Build firmware
pio run

# Build filesystem image
pio run --target buildfs

# Firmware binary will be at: .pio/build/seeed_xiao_esp32s3/firmware.bin
# Filesystem binary will be at: .pio/build/seeed_xiao_esp32s3/littlefs.bin
```

### GitHub Actions

The project includes a GitHub Actions workflow that:
- Builds the firmware on every tag push
- Creates a release with firmware binaries
- Can be manually triggered via workflow_dispatch

To create a release:
```bash
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

## API Reference

This project uses the [OneBusAway API](https://developer.onebusaway.org/):
- Endpoint: `/api/where/trips-for-route/[ROUTE_ID].json`
- Documentation: https://developer.onebusaway.org/

## Troubleshooting

### WiFi Connection Issues

- If the ESP32 fails to connect, it will restart and create the `LinkLight-Setup` access point again
- The portal timeout is set to 180 seconds (3 minutes)

### LED Not Working

- Check that the LED data pin is correctly connected to GPIO 8 (or your configured pin)
- Verify power supply is adequate for your LED strip
- Some LED strips may require a level shifter for the data line

### Serial Monitor

Connect at 115200 baud to see debug output:
```bash
pio device monitor -b 115200
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- SoundTransit for providing the Open Transit Data API
- OneBusAway project for the API infrastructure
- All the open-source libraries used in this project
