#include "sensor_manager.h"
#include <WiFi.h>

void SensorManager::updateSensorData(const String &senderIP, const String &clientId, int sensorValue)
{
    sensorDataMap[senderIP] = {clientId, sensorValue};
    // Serial.printf("Sensor update from %s: clientId=%s, value=%d\n", senderIP.c_str(), clientId.c_str(), sensorValue);
}

String SensorManager::getSensorDataJSON() const
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

    return json;
}

const std::map<String, SensorData> &SensorManager::getAllSensorData() const
{
    return sensorDataMap;
}

void SensorManager::clearSensorData()
{
    sensorDataMap.clear();
}

bool SensorManager::hasSensorData() const
{
    return !sensorDataMap.empty();
}

String SensorManager::getFormattedSensorData() const
{
    String result = "TP:";
    bool first = true;

    for (const auto &pair : sensorDataMap)
    {
        if (!first)
        {
            result += ",";
        }
        result += String(pair.second.value);
        first = false;
    }

    return result;
}

String SensorManager::getFormattedSensorData(int minSensors) const
{
    String result = "TP:";
    bool first = true;
    int sensorCount = 0;

    // Add actual sensor values
    for (const auto &pair : sensorDataMap)
    {
        if (!first)
        {
            result += ",";
        }
        result += String(pair.second.value);
        first = false;
        sensorCount++;
    }

    // Pad with zeros if we have fewer sensors than minSensors
    while (sensorCount < minSensors)
    {
        if (!first)
        {
            result += ",";
        }
        result += "0";
        first = false;
        sensorCount++;
    }

    return result;
}

// Generate a unique client ID using the ESP's MAC address
String getUniqueClientId()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char clientId[13];
    snprintf(clientId, sizeof(clientId), "ESP_%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String(clientId);
}

// Add a method to get this device's own sensor value
int SensorManager::getLocalSensorValue() const
{
// If you have a dedicated sensor pin, read it here. For example, using touchRead or analogRead.
// Example for a touch sensor on GPIO4:
#define TOUCH_PIN 4
#define TOUCH_THRESHOLD 40
    int touchValue = touchRead(TOUCH_PIN);
    int sensorValue = (touchValue < TOUCH_THRESHOLD) ? 1 : 0;
    return sensorValue;
}

// New method to get local sensor data as JSON
String SensorManager::getLocalSensorDataJSON() const
{
    String json = "{";

    // Get local IP address
    String localIP = WiFi.localIP().toString();
    json += "\"ip\":\"" + localIP + "\",";

    // Get unique client ID
    String clientId = getUniqueClientId();
    json += "\"clientId\":\"" + clientId + "\",";

    // Get local sensor value
    int sensorValue = getLocalSensorValue();
    json += "\"value\":" + String(sensorValue);

    json += "}";
    return json;
}