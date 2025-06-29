#include <Arduino.h>
#include <WebServer.h>
#include <HTTPClient.h>

// Project headers
#include "config.h"
#include "sensor_manager.h"
#include "web_handlers.h"
#include "wifi_manager.h"
#include "filesystem_utils.h"

// Global objects
SensorManager sensorManager;
WebServer server(WEB_SERVER_PORT);
WebHandlers webHandlers(&server, &sensorManager);
WiFiManager wifiManager; // No LEDController

// --- Client-only additions ---
const char *serverUrl = "http://192.168.1.200/sensor"; // Central server endpoint
unsigned long lastSensorSend = 0;
const long sendInterval = 200; // ms

// Generate a unique client ID using the ESP's MAC address
String getUniqueClientId()
{
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char clientId[13];
  snprintf(clientId, sizeof(clientId), "ESP_%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(clientId);
}

void setup()
{
  // Initialize Serial for debugging
  Serial.begin(115200);
  Serial.println("\n\nESP32-S3 Client Starting...");

  // Initialize file system
  if (!FilesystemUtils::initSPIFFS())
  {
    Serial.println("Failed to initialize SPIFFS");
    return;
  }

  // List files and check for index.html
  FilesystemUtils::listFiles();
  FilesystemUtils::checkIndexFile();

  // Initialize WiFi
  if (wifiManager.init())
  {
    // Setup web server routes (for local HTML, update, etc.)
    webHandlers.setupRoutes();
    server.begin();
    Serial.println("HTTP server started");
    Serial.println("Setup completed successfully!");
  }
  else
  {
    Serial.println("WiFi initialization failed!");
  }
}

void loop()
{
  unsigned long currentMillis = millis();

  // Handle WiFi connection and OTA
  wifiManager.handleConnection();

  // Handle web server requests only if connected
  if (wifiManager.isConnected())
  {
    server.handleClient();
  }

  // --- Send our own sensor data to the central server ---
  if (wifiManager.isConnected() && (currentMillis - lastSensorSend >= sendInterval))
  {
    int sensorValue = sensorManager.getLocalSensorValue(); // You may need to implement this if not present
    String clientId = getUniqueClientId();

    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postData = "clientId=" + clientId + "&value=" + String(sensorValue);
    int httpResponseCode = http.POST(postData);
    if (httpResponseCode == 200)
    {
      Serial.printf("[SEND] Value: %d\n", sensorValue);
    }
    else
    {
      Serial.printf("[SEND ERROR] %d: %s\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
    }
    http.end();
    lastSensorSend = currentMillis;
  }

  // --- Display our own sensor data locally ---
  // (Assume sensorManager.getLocalSensorValue() returns this device's value)
  static unsigned long lastPrint = 0;
  if (currentMillis - lastPrint >= 1000)
  {
    Serial.printf("Local Sensor Value: %d\n", sensorManager.getLocalSensorValue());
    lastPrint = currentMillis;
  }
}