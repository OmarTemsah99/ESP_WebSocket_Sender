#include "sensor_manager.h"

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