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