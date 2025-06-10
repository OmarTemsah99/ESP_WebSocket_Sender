#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WebServer.h>
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
  int value;
  unsigned long lastUpdate;
};
std::map<String, SensorData> sensorDataMap;

// HTML page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Control Center</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; margin: 20px; }
        .slider { width: 200px; }
        .color-preview {
            width: 100px;
            height: 100px;
            margin: 20px auto;
            border: 2px solid #333;
        }
        .sensor-data {
            margin: 20px;
            padding: 10px;
            border: 1px solid #ccc;
            border-radius: 5px;
        }
        .sensor {
            margin: 10px;
            padding: 10px;
            background-color: #f0f0f0;
            border-radius: 5px;
        }
    </style>
</head>
<body>
    <h1>ESP32 Control Center</h1>
    <div class="color-preview" id="colorPreview"></div>
    <div class="rgb-control">
        <h2>RGB LED Control</h2>
        <p>
            Red: <input type="range" class="slider" id="red" min="0" max="255" value="0" oninput="updateColor()"><br>
            Green: <input type="range" class="slider" id="green" min="0" max="255" value="0" oninput="updateColor()"><br>
            Blue: <input type="range" class="slider" id="blue" min="0" max="255" value="0" oninput="updateColor()">
        </p>
    </div>
    <div class="sensor-data">
        <h2>Sensor Data</h2>
        <div id="sensorList"></div>
    </div>
    <script>
        function updateColor() {
            var r = document.getElementById('red').value;
            var g = document.getElementById('green').value;
            var b = document.getElementById('blue').value;
            document.getElementById('colorPreview').style.backgroundColor = 
                'rgb(' + r + ',' + g + ',' + b + ')';
            fetch('/color?r=' + r + '&g=' + g + '&b=' + b)
                .catch(error => console.error('Error:', error));
        }

        function updateSensorData() {
            fetch('/sensorData')
                .then(response => response.json())
                .then(data => {
                    const sensorList = document.getElementById('sensorList');
                    sensorList.innerHTML = '';
                    for (const [ip, sensorInfo] of Object.entries(data)) {
                        const sensorDiv = document.createElement('div');
                        sensorDiv.className = 'sensor';
                        const age = Math.floor((Date.now() - sensorInfo.lastUpdate) / 1000);
                        sensorDiv.innerHTML = `
                            <strong>ESP ID (IP): ${ip}</strong><br>
                            Sensor Value: ${sensorInfo.value}<br>
                            Last Update: ${age} seconds ago
                        `;
                        sensorList.appendChild(sensorDiv);
                    }
                })
                .catch(error => console.error('Error:', error));
        }

        // Update sensor data every second
        setInterval(updateSensorData, 1000);
    </script>
</body>
</html>
)rawliteral";

// Handle sensor data submission
void handleSensorData()
{
  String senderIP = server.client().remoteIP().toString();
  int sensorValue = server.arg("value").toInt();

  // Update sensor data
  sensorDataMap[senderIP] = {
      sensorValue,
      millis()};

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
    json += "\"value\":" + String(pair.second.value) + ",";
    json += "\"lastUpdate\":" + String(pair.second.lastUpdate);
    json += "}";
    first = false;
  }
  json += "}";

  server.send(200, "application/json", json);
}

void handleRoot()
{
  server.send(200, "text/html", index_html);
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
  // Initialize NeoPixel first for immediate visual feedback
  pixel.begin();
  pixel.setBrightness(128);
  setDisconnectedIndicator();

  // Initialize Serial without waiting
  Serial.begin(115200);
  Serial.println("\n\nESP32-S3 NeoPixel Web Server Starting...");
  Serial.println("NeoPixel Initialized");
  Serial.flush();

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
  Serial.flush();

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
    Serial.flush();
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
    server.on("/", handleRoot);
    server.on("/color", handleColor);
    server.on("/sensor", HTTP_POST, handleSensorData);
    server.on("/sensorData", HTTP_GET, handleGetSensorData);

    // Start web server
    server.begin();
    Serial.println("HTTP server started");
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
  Serial.flush();
}

void loop()
{
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