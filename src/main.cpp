#include <Arduino.h>
#include <WebServer.h>
#include <HTTPClient.h>

// Project headers
#include "config.h"
#include "sensor_manager.h"
#include "web_handlers.h"
#include "wifi_manager.h"
#include "filesystem_utils.h"

// ========================= GLOBAL OBJECTS =========================
SensorManager sensorManager;
WebServer server(WEB_SERVER_PORT);
WebHandlers webHandlers(&server, &sensorManager);
WiFiManager wifiManager;

// ========================= CLIENT CONFIGURATION =========================
const char *SERVER_URL = "http://192.168.1.200/sensor";
const unsigned long SEND_INTERVAL = 200; // ms
int clientId = 0;                        // Will be set via web interface

// ========================= TIMING VARIABLES =========================
unsigned long lastSensorSend = 0;
unsigned long lastLocalDisplay = 0;

// ========================= HELPER FUNCTIONS =========================

void sendSensorDataToServer()
{
  int sensorValue = sensorManager.getLocalSensorValue();

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "clientId=" + String(clientId) + "&value=" + String(sensorValue);
  int responseCode = http.POST(postData);

  if (responseCode == 200)
  {
    Serial.printf("[SEND] ID: %d, Value: %d\n", clientId, sensorValue);
  }
  else
  {
    Serial.printf("[SEND ERROR] %d: %s\n", responseCode, http.errorToString(responseCode).c_str());
  }

  http.end();
}

void displayLocalSensorData()
{
  int sensorValue = sensorManager.getLocalSensorValue();
  Serial.printf("Local Sensor Value: %d\n", sensorValue);
}

bool initializeSystem()
{
  // Initialize Serial
  Serial.begin(115200);
  Serial.println("\n=== ESP32-S3 Client Starting ===");

  // Initialize filesystem
  if (!FilesystemUtils::initSPIFFS())
  {
    Serial.println("ERROR: Failed to initialize SPIFFS");
    return false;
  }

  // Check filesystem contents
  FilesystemUtils::listFiles();
  FilesystemUtils::checkIndexFile();

  // Initialize WiFi and web server
  if (!wifiManager.init())
  {
    Serial.println("ERROR: WiFi initialization failed");
    return false;
  }

  webHandlers.setupRoutes(clientId);
  server.begin();

  Serial.println("=== System initialized successfully ===");
  Serial.printf("Web server running on: http://%s\n", WiFi.localIP().toString().c_str());

  return true;
}

// ========================= ARDUINO FUNCTIONS =========================

void setup()
{
  if (!initializeSystem())
  {
    Serial.println("FATAL: System initialization failed!");
    while (true)
      delay(1000); // Stop execution
  }
}

void loop()
{
  unsigned long currentTime = millis();

  // Handle WiFi connection and OTA
  wifiManager.handleConnection();

  // Handle web server requests (only when connected)
  if (wifiManager.isConnected())
  {
    server.handleClient();

    // Send sensor data to central server
    if (currentTime - lastSensorSend >= SEND_INTERVAL)
    {
      sendSensorDataToServer();
      lastSensorSend = currentTime;
    }
  }

  // Display local sensor data (every second)
  if (currentTime - lastLocalDisplay >= 1000)
  {
    displayLocalSensorData();
    lastLocalDisplay = currentTime;
  }
}