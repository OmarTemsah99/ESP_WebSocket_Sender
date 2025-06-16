# ESP32-S3 Sensor Monitor

A comprehensive ESP32-S3 based sensor monitoring system with web interface, file management, and over-the-air (OTA) firmware updates. Features real-time sensor data visualization, RGB LED status indicators, and a complete web-based management interface.

## Features

- **Real-time Sensor Monitoring**: Collect and display sensor data from multiple clients
- **Web Interface**: Clean, responsive web UI for monitoring and management
- **File Management**: Upload, download, and delete files on the SPIFFS filesystem
- **OTA Updates**: Over-the-air firmware updates via web interface
- **RGB LED Status**: Visual indicators for connection status and sensor states
- **Static IP Configuration**: Reliable network connectivity
- **RESTful API**: JSON-based API for sensor data integration

## Hardware Requirements

- ESP32-S3 Development Board (tested on ESP32-S3-DevKitC-1)
- WS2812B RGB LED (NeoPixel) connected to GPIO 48
- Stable power supply
- WiFi network access

## Pin Configuration

```
RGB LED (WS2812B): GPIO 48
Number of pixels: 1
```

## Software Dependencies

### PlatformIO Libraries

- `adafruit/Adafruit NeoPixel @ ^1.11.0`

### Built-in Libraries

- WiFi
- WebServer
- SPIFFS
- ArduinoOTA
- Update

## Installation

### 1. Hardware Setup

- Connect the WS2812B RGB LED to GPIO 48
- Ensure proper power supply to the ESP32-S3

### 2. Configuration

Create a `config.cpp` file in the `src/` directory with your network settings:

```cpp
#include "config.h"

// WiFi credentials
const char *WIFI_SSID = "your_wifi_ssid";
const char *WIFI_PASSWORD = "your_wifi_password";

// Static IP configuration
const IPAddress STATIC_IP(192, 168, 1, 200);
const IPAddress GATEWAY(192, 168, 1, 1);
const IPAddress SUBNET(255, 255, 255, 0);
const IPAddress DNS_SERVER(8, 8, 8, 8);
```

### 3. Build and Upload

```bash
# Install PlatformIO CLI
pip install platformio

# Clone the repository
git clone <repository_url>
cd esp32-sensor-monitor

# Build and upload via USB
pio run -t upload -e esp32-s3-usb

# Upload filesystem data
pio run -t uploadfs -e esp32-s3-usb
```

### 4. OTA Updates (After Initial Setup)

```bash
# For OTA updates (update the IP address in platformio.ini)
pio run -t upload -e esp32-s3-ota
```

## Usage

### Web Interface

After successful connection, access the web interface at:

- **Main Dashboard**: `http://192.168.1.200/` (or your configured IP)
- **File Manager**: `http://192.168.1.200/upload`
- **Firmware Update**: `http://192.168.1.200/firmware`

### LED Status Indicators

- **Red**: Disconnected from WiFi
- **Blue (Blinking)**: Connecting to WiFi
- **Green**: Connected and ready
- **Blue (Solid)**: Sensor value = 1
- **Red (Solid)**: Sensor value = 0

### API Endpoints

#### Sensor Data

```http
POST /sensor
Content-Type: application/x-www-form-urlencoded

clientId=sensor1&value=1
```

#### Get Sensor Data

```http
GET /sensorData
Response: {"192.168.1.100": {"clientId": "sensor1", "value": 1}}
```

#### Control LED Color

```http
GET /color?r=255&g=0&b=0
```

#### File Operations

```http
GET /list                    # List files
POST /delete?file=filename   # Delete file
POST /upload                 # Upload file (multipart/form-data)
```

## File Structure

```
├── data/                    # SPIFFS filesystem data
│   ├── index.html          # Main dashboard
│   ├── file_manager.html   # File management interface
│   ├── firmware_update.html # Firmware update interface
│   └── styles.css          # Stylesheet
├── include/                # Header files
│   ├── config.h           # Configuration constants
│   ├── led_controller.h   # LED management
│   ├── sensor_manager.h   # Sensor data handling
│   ├── web_handlers.h     # Web server routes
│   ├── wifi_manager.h     # WiFi and OTA management
│   └── filesystem_utils.h # SPIFFS utilities
├── src/                   # Source files
│   ├── main.cpp          # Main application
│   ├── led_controller.cpp
│   ├── sensor_manager.cpp
│   ├── web_handlers.cpp
│   ├── wifi_manager.cpp
│   └── filesystem_utils.cpp
└── platformio.ini        # PlatformIO configuration
```

## Configuration Options

### Network Settings

- Modify IP addresses in `config.cpp`
- Update OTA settings in `platformio.ini`

### Hardware Pins

- Change `RGB_LED_PIN` in `config.h`
- Adjust `NUM_PIXELS` for multiple LEDs

### Timing

- `RECONNECT_INTERVAL`: WiFi reconnection delay (default: 10 seconds)
- `SENSOR_UPDATE_INTERVAL`: Sensor data refresh rate (default: 50ms)

## Development

### Adding New Features

1. **New Web Routes**: Add handlers in `web_handlers.cpp`
2. **Sensor Types**: Extend `SensorManager` class
3. **LED Patterns**: Add methods to `LEDController`

### Building for Production

```bash
# Optimized build
pio run -e esp32-s3-ota
```

## Troubleshooting

### Common Issues

1. **WiFi Connection Failed**

   - Check credentials in `config.cpp`
   - Verify network settings
   - Check signal strength

2. **SPIFFS Mount Failed**

   - Upload filesystem: `pio run -t uploadfs`
   - Format SPIFFS via serial monitor

3. **OTA Update Failed**

   - Ensure stable power supply
   - Check IP address configuration
   - Verify `.bin` file integrity

4. **Web Interface Not Loading**
   - Verify `index.html` exists in SPIFFS
   - Check file permissions
   - Monitor serial output for errors

### Serial Monitor Commands

```
Monitor at 115200 baud for debug information:
- WiFi connection status
- File system operations
- Sensor data updates
- Error messages
```

## Safety Considerations

- **Firmware Updates**: Ensure stable power during OTA updates
- **File Operations**: Backup important files before deletion
- **Network Security**: Change default OTA password in production
- **Power Supply**: Use adequate power supply for stable operation

## License

This project is open source. Please check the license file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Support

For issues and questions:

- Check the troubleshooting section
- Review serial monitor output
- Submit issues with detailed error logs
