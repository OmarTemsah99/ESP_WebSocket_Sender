#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define TOUCH_PIN 4        // Using Touch0 which is GPIO4
#define TOUCH_THRESHOLD 40 // Adjust this value based on your needs

const char *ssid = "";
const char *password = "";
const char *serverUrl = "http://192.168.1.200/sensor"; // Your first ESP's IP

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
  Serial.begin(115200);
  Serial.println("\nESP32 Sensor Client Starting...");

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Client ID: ");
  Serial.println(getUniqueClientId());
}

void loop()
{
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();

  // Update every 50ms (20 times per second)
  if (currentMillis - lastUpdate >= 50)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;

      // Read touch sensor
      int touchValue = touchRead(TOUCH_PIN);
      int sensorValue = (touchValue < TOUCH_THRESHOLD) ? 1 : 0;

      // Send data to server
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String clientId = getUniqueClientId();
      String postData = "clientId=" + clientId + "&value=" + String(sensorValue);

      int httpResponseCode = http.POST(postData);

      if (httpResponseCode == 200)
      {
        Serial.printf("Touch: %d, Value: %d\n", touchValue, sensorValue);
      }
      else
      {
        Serial.printf("Error %d: %s\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
      }

      http.end();
    }
    else
    {
      Serial.println("WiFi Disconnected! Attempting to reconnect...");
      WiFi.begin(ssid, password);
    }

    lastUpdate = currentMillis;
  }

  // Small delay to prevent overwhelming the CPU
  delay(200);
}
