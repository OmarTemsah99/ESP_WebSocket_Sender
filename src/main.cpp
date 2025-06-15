#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiUDP.h>
#include <ArduinoOTA.h>
#include <SPIFFS.h>
#include <map>
#include <string>

// WiFi credentials
const char *ssid = "TemsahDSLX";
const char *password = "#Tt01125115502";

// Static IP configuration
IPAddress staticIP(192, 168, 1, 200); // Change this to your desired static IP
IPAddress gateway(192, 168, 1, 2);    // Updated router's IP address
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress dns(8, 8, 8, 8);            // DNS server (using Google's DNS)

// RGB LED Pin definitions
#define RGB_LED 48
#define NUM_PIXELS 1

// Create NeoPixel object
Adafruit_NeoPixel pixel(NUM_PIXELS, RGB_LED, NEO_GRB + NEO_KHZ800);

// Create web server object
WebServer server(80);

// Current LED color values
int currentRed = 0;
int currentGreen = 0;
int currentBlue = 0;

// LED indication colors
void setConnectingIndicator()
{
  static bool ledState = false;
  ledState = !ledState;
  pixel.setPixelColor(0, ledState ? pixel.Color(0, 0, 255) : pixel.Color(0, 0, 0)); // Blink bright blue
  pixel.show();
}

void setConnectedIndicator()
{
  pixel.setPixelColor(0, pixel.Color(0, 255, 0)); // Bright solid green
  pixel.show();
}

void setDisconnectedIndicator()
{
  pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Solid red for disconnected
  pixel.show();
}

// Store sensor data from different ESPs
struct SensorData
{
  String clientId;
  int value;
};
std::map<String, SensorData> sensorDataMap; // Key is IP address

// Handle root path
void handleRoot()
{
  if (!SPIFFS.exists("/index.html"))
  {
    Serial.println("Error: index.html not found in SPIFFS");
    server.send(500, "text/plain", "File not found in SPIFFS");
    return;
  }

  File file = SPIFFS.open("/index.html", "r");
  if (!file)
  {
    Serial.println("Error: Failed to open index.html");
    server.send(500, "text/plain", "Failed to open file");
    return;
  }

  size_t fileSize = file.size();
  Serial.printf("File size: %d bytes\n", fileSize);

  if (fileSize == 0)
  {
    Serial.println("Error: index.html is empty");
    server.send(500, "text/plain", "File is empty");
    file.close();
    return;
  }

  server.sendHeader("Content-Length", String(fileSize));
  server.setContentLength(fileSize);
  server.send(200, "text/html", ""); // Send header first

  // Send file in chunks
  const size_t bufSize = 1024;
  uint8_t buf[bufSize];
  size_t totalSent = 0;

  while (totalSent < fileSize)
  {
    size_t toRead = min(bufSize, fileSize - totalSent);
    size_t bytesRead = file.read(buf, toRead);
    if (bytesRead == 0)
    {
      Serial.println("Error: Failed to read file");
      break;
    }
    server.sendContent((char *)buf, bytesRead);
    totalSent += bytesRead;
  }

  file.close();
  Serial.printf("File sent successfully. Total bytes: %d\n", totalSent);
}

// Handle sensor data submission
void handleSensorData()
{
  String senderIP = server.client().remoteIP().toString();
  String clientId = server.arg("clientId");
  int sensorValue = server.arg("value").toInt();

  // Update sensor data
  sensorDataMap[senderIP] = {
      clientId,
      sensorValue};

  // If sensor value is 1, set LED to blue, if 0 set to red
  if (sensorValue == 1)
  {
    pixel.setPixelColor(0, pixel.Color(0, 0, 255)); // Blue
  }
  else
  {
    pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red
  }
  pixel.show();

  server.send(200, "text/plain", "OK");
}

// Handle sensor data request from web page
void handleGetSensorData()
{
  String json = "{";
  bool first = true;

  for (const auto &pair : sensorDataMap)
  {
    if (!first)
    {
      json += ",";
    }
    json += "\"" + pair.first + "\":{";
    json += "\"clientId\":\"" + pair.second.clientId + "\",";
    json += "\"value\":" + String(pair.second.value);
    json += "}";
    first = false;
  }
  json += "}";

  server.send(200, "application/json", json);
}

void handleColor()
{
  currentRed = server.arg("r").toInt();
  currentGreen = server.arg("g").toInt();
  currentBlue = server.arg("b").toInt();

  pixel.setPixelColor(0, pixel.Color(currentRed, currentGreen, currentBlue));
  pixel.show();

  server.send(200, "text/plain", "OK");
}

void setup()
{
  // Initialize Serial first for debugging
  Serial.begin(115200);
  Serial.println("\n\nESP32-S3 NeoPixel Web Server Starting...");

  // Initialize SPIFFS with detailed logging
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Mount Failed. Trying to format...");
    if (!SPIFFS.format())
    {
      Serial.println("SPIFFS Format Failed");
      return;
    }
    if (!SPIFFS.begin())
    {
      Serial.println("SPIFFS Mount Failed after formatting");
      return;
    }
  }
  Serial.println("SPIFFS mounted successfully");

  // List all files in SPIFFS
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  Serial.println("\nFiles in SPIFFS:");
  while (file)
  {
    String fileName = file.name();
    size_t fileSize = file.size();
    Serial.printf("- File: %s, Size: %d bytes\n", fileName.c_str(), fileSize);
    file = root.openNextFile();
  }
  root.close();

  // Try to open index.html specifically
  if (SPIFFS.exists("/index.html"))
  {
    File indexFile = SPIFFS.open("/index.html", "r");
    if (indexFile)
    {
      Serial.printf("index.html found, size: %d bytes\n", indexFile.size());
      indexFile.close();
    }
    else
    {
      Serial.println("index.html exists but cannot be opened");
    }
  }
  else
  {
    Serial.println("index.html not found in SPIFFS");
  }

  // Initialize NeoPixel
  pixel.begin();
  pixel.setBrightness(128);
  setDisconnectedIndicator();

  // Initialize Serial without waiting
  Serial.begin(115200);
  Serial.println("\n\nESP32-S3 NeoPixel Web Server Starting...");
  Serial.println("NeoPixel Initialized");

  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  // Set WiFi mode to station (client)
  WiFi.mode(WIFI_STA);

  // Configure static IP before connecting
  Serial.println("Configuring Static IP...");
  if (!WiFi.config(staticIP, gateway, subnet, dns))
  {
    Serial.println("Static IP Configuration Failed!");
  }
  else
  {
    Serial.println("Static IP Configuration Successful");
  }

  // Connect to WiFi
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);

  // Add timeout for WiFi connection
  int connectionAttempts = 0;
  const int maxAttempts = 20; // 10 seconds timeout (20 * 500ms)

  while (WiFi.status() != WL_CONNECTED && connectionAttempts < maxAttempts)
  {
    setConnectingIndicator(); // Blink blue while connecting
    delay(500);
    Serial.print(".");
    connectionAttempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    setConnectedIndicator(); // Solid green when connected
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    // Setup web server routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/sensor", HTTP_POST, handleSensorData);
    server.on("/sensorData", HTTP_GET, handleGetSensorData);
    server.begin();
    Serial.println("HTTP server started");

    // Configure OTA
    ArduinoOTA.setHostname("ESP32-Sensor-Monitor");
    ArduinoOTA.setPassword("admin"); // Set OTA password

    ArduinoOTA.onStart([]()
                       {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
      {
        type = "sketch";
      }
      else
      {
        type = "filesystem";
      }
      Serial.println("Start updating " + type); });

    ArduinoOTA.onEnd([]()
                     { Serial.println("\nEnd"); });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });

    ArduinoOTA.onError([](ota_error_t error)
                       {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)
        Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)
        Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)
        Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)
        Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)
        Serial.println("End Failed"); });

    ArduinoOTA.begin();
    Serial.println("OTA Ready");
  }
  else
  {
    setDisconnectedIndicator(); // LED off if connection failed
    Serial.println("Failed to connect to WiFi!");
    Serial.print("WiFi Status: ");
    switch (WiFi.status())
    {
    case WL_IDLE_STATUS:
      Serial.println("IDLE");
      break;
    case WL_NO_SSID_AVAIL:
      Serial.println("SSID not available");
      break;
    case WL_SCAN_COMPLETED:
      Serial.println("Scan completed");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("Connection failed");
      break;
    case WL_CONNECTION_LOST:
      Serial.println("Connection lost");
      break;
    case WL_DISCONNECTED:
      Serial.println("Disconnected");
      break;
    default:
      Serial.println("Unknown status");
    }
  }
}

void loop()
{
  ArduinoOTA.handle(); // Handle OTA updates

  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED)
  {
    static unsigned long lastAttempt = 0;
    unsigned long currentMillis = millis();

    // Try to reconnect every 10 seconds
    if (currentMillis - lastAttempt > 10000)
    {
      Serial.println("WiFi connection lost! Reconnecting...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      lastAttempt = currentMillis;
    }
    setConnectingIndicator(); // Blink blue while attempting to reconnect
  }
  else
  {
    server.handleClient();
  }
}