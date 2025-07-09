// ==================== main.cpp ====================
#include <Arduino.h>
#include <WebServer.h>
#include <HTTPClient.h>

// Project headers
#include "config.h"
#include "sensor_manager.h"
#include "web_handlers.h"
#include "wifi_manager.h"
#include "filesystem_utils.h"
#include "client_config.h"
#include "ClientIdentity.h"

// ========================= GLOBAL OBJECTS =========================
SensorManager sensorManager;
WebServer server(WEB_SERVER_PORT);
WiFiManager wifiManager;
ClientConfig clientConfig;
ClientIdentity clientIdentity(&clientConfig);
WebHandlers webHandlers(&server, &sensorManager, &clientIdentity);

// ========================= GLOBAL VARIABLES =========================
#define BTN_INC_PIN 4
#define BTN_DEC_PIN 15

int buttonStateInc = HIGH;
int lastButtonStateInc = HIGH;
unsigned long lastDebounceTimeInc = 0;

int buttonStateDec = HIGH;
int lastButtonStateDec = HIGH;
unsigned long lastDebounceTimeDec = 0;

const unsigned long debounceDelay = 50;

// ========================= CLIENT CONFIGURATION =========================
const char *SERVER_URL = "http://192.168.1.200/sensor";
const unsigned long SEND_INTERVAL = 200; // ms

// ========================= TIMING VARIABLES =========================
unsigned long lastSensorSend = 0;
unsigned long lastLocalDisplay = 0;

// ========================= HELPER FUNCTIONS =========================
void handleButton(int &lastState, int &buttonState, unsigned long &lastTime, int pin, int direction)
{
  int reading = digitalRead(pin);

  if (reading != lastState)
  {
    lastTime = millis(); // Reset the debounce timer
  }

  if ((millis() - lastTime) > debounceDelay)
  {
    if (reading != buttonState)
    {
      buttonState = reading;

      // Only trigger on button press (LOW for pull-up or HIGH for pull-down)
      if (buttonState == HIGH)
      {
        int id = clientIdentity.get();
        id += direction;
        id = constrain(id, 0, 15);
        clientIdentity.set(id);
        Serial.printf("[BUTTON] Client ID %s to %d\n", (direction > 0 ? "increased" : "decreased"), id);
      }
    }
  }

  lastState = reading;
}

void sendSensorDataToServer()
{
  int touchValue = sensorManager.getLocalTouchValue();
  float batteryVoltage = sensorManager.getLocalBatteryVoltage();
  float batteryPercent = sensorManager.getLocalBatteryPercent();
  int clientId = clientIdentity.get();

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "clientId=" + String(clientId) +
                    "&touch=" + String(touchValue) +
                    "&batteryVoltage=" + String(batteryVoltage, 2) +
                    "&batteryPercent=" + String(batteryPercent, 1);
  int responseCode = http.POST(postData);

  if (responseCode == 200)
  {
    Serial.printf("[SEND] ID: %d, Touch: %d, Battery: %.2fV (%.1f%%)\n", clientId, touchValue, batteryVoltage, batteryPercent);
  }
  else
  {
    Serial.printf("[SEND ERROR] %d: %s\n", responseCode, http.errorToString(responseCode).c_str());
  }

  http.end();
}

void displayLocalSensorData()
{
  int touchValue = sensorManager.getLocalTouchValue();
  float batteryVoltage = sensorManager.getLocalBatteryVoltage();
  float batteryPercent = sensorManager.getLocalBatteryPercent();
  Serial.printf("Local Touch: %d, Battery: %.2fV (%.1f%%)\n", touchValue, batteryVoltage, batteryPercent);
}

bool initializeSystem()
{
  Serial.begin(115200);
  Serial.println("\n=== ESP32-S3 Client Starting ===");

  clientIdentity.begin();
  sensorManager.begin(&clientIdentity);

  if (!FilesystemUtils::initSPIFFS())
  {
    Serial.println("ERROR: Failed to initialize SPIFFS");
    return false;
  }

  FilesystemUtils::listFiles();
  FilesystemUtils::checkIndexFile();

  if (!wifiManager.init())
  {
    Serial.println("ERROR: WiFi initialization failed");
    return false;
  }

  webHandlers.setupRoutes();
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
      delay(1000);
  }

  pinMode(BTN_INC_PIN, INPUT);
  pinMode(BTN_DEC_PIN, INPUT);
}

void loop()
{
  handleButton(lastButtonStateInc, buttonStateInc, lastDebounceTimeInc, BTN_INC_PIN, +1);
  handleButton(lastButtonStateDec, buttonStateDec, lastDebounceTimeDec, BTN_DEC_PIN, -1);

  unsigned long currentTime = millis();
  wifiManager.handleConnection();

  if (wifiManager.isConnected())
  {
    server.handleClient();
    if (currentTime - lastSensorSend >= SEND_INTERVAL)
    {
      sendSensorDataToServer();
      lastSensorSend = currentTime;
    }
  }

  // if (currentTime - lastLocalDisplay >= SEND_INTERVAL)
  // {
  //   displayLocalSensorData();
  //   lastLocalDisplay = currentTime;
  // }
}