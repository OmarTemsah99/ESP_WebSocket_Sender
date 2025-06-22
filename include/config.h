#ifndef CONFIG_H
#define CONFIG_H

#include <IPAddress.h>

// WiFi credentials
extern const char *WIFI_SSID;
extern const char *WIFI_PASSWORD;

// Static IP configuration
extern const IPAddress STATIC_IP;
extern const IPAddress GATEWAY;
extern const IPAddress SUBNET;
extern const IPAddress DNS_SERVER;

// Hardware pin definitions
#define RGB_LED_PIN 48
#define NUM_PIXELS 1

// Server configuration
#define WEB_SERVER_PORT 80
#define OTA_PASSWORD "admin"
#define OTA_HOSTNAME "ESP32-Sensor-Monitor"

// Timing constants
#define RECONNECT_INTERVAL 10000   // 10 seconds
#define SENSOR_UPDATE_INTERVAL 200 // 200ms
#define CONNECTION_TIMEOUT 20      // 20 attempts (10 seconds)

#endif // CONFIG_H