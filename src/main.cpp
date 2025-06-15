#include <Arduino.h>
#include <WebServer.h>

// Project headers
#include "config.h"
#include "led_controller.h"
#include "sensor_manager.h"
#include "web_handlers.h"
#include "wifi_manager.h"
#include "filesystem_utils.h"

// Global objects
LEDController ledController;
SensorManager sensorManager;
WebServer server(WEB_SERVER_PORT);
WebHandlers webHandlers(&server, &sensorManager, &ledController);
WiFiManager wifiManager(&ledController);

void setup()
{
  // Initialize Serial for debugging
  Serial.begin(115200);
  Serial.println("\n\nESP32-S3 NeoPixel Web Server Starting...");

  // Initialize LED controller first for status indication
  ledController.init();
  Serial.println("LED Controller initialized");

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
    // Setup web server routes
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
  // Handle WiFi connection and OTA
  wifiManager.handleConnection();

  // Handle web server requests only if connected
  if (wifiManager.isConnected())
  {
    server.handleClient();
  }
}