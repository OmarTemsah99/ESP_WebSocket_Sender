#include "sensor_manager.h"
#include <WiFi.h>

#define TOUCH_PIN 13
#define BATTERY_PIN 34
#define TOUCH_THRESHOLD 40
#define VCC 3.3f
#define R1 100000.0f            // Adjust as per your voltage divider
#define R2 10000.0f             // Adjust as per your voltage divider
#define CALIBRATION_FACTOR 1.0f // Adjust as needed

void SensorManager::updateSensorData(const String &senderIP, const String &clientId, int touchValue, float batteryVoltage, float batteryPercent)
{
    sensorDataMap[senderIP] = {clientId, touchValue, batteryVoltage, batteryPercent};
}

String SensorManager::getSensorDataJSON() const
{
    String json = "{";
    bool first = true;
    for (const auto &pair : sensorDataMap)
    {
        if (!first)
            json += ",";
        json += "\"" + pair.first + "\":{";
        json += "\"clientId\":\"" + pair.second.clientId + "\",";
        json += "\"touch\":" + String(pair.second.touchValue) + ",";
        json += "\"batteryVoltage\":" + String(pair.second.batteryVoltage, 2) + ",";
        json += "\"batteryPercent\":" + String(pair.second.batteryPercent, 1);
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
            result += ",";
        result += String(pair.second.touchValue) + "," + String(pair.second.batteryPercent, 1);
        first = false;
    }
    return result;
}

String SensorManager::getFormattedSensorData(int minSensors) const
{
    String result = "TP:";
    bool first = true;
    int sensorCount = 0;
    for (const auto &pair : sensorDataMap)
    {
        if (!first)
            result += ",";
        result += String(pair.second.touchValue) + "," + String(pair.second.batteryPercent, 1);
        first = false;
        sensorCount++;
    }
    while (sensorCount < minSensors)
    {
        if (!first)
            result += ",";
        result += "0,0.0";
        first = false;
        sensorCount++;
    }
    return result;
}

int SensorManager::getLocalTouchValue() const
{
    int touchValue = digitalRead(TOUCH_PIN);
    return touchValue;
}

float SensorManager::getLocalBatteryVoltage() const
{
    float totalVoltage = 0.0f;
    for (int i = 0; i < 100; i++)
    {
        int batteryReading = analogRead(BATTERY_PIN);
        float batteryVoltage = batteryReading * ((VCC / 4096.0f) * (R1 + R2) / R2) * CALIBRATION_FACTOR;
        totalVoltage += batteryVoltage;
    }
    float averageVoltage = totalVoltage / 100.0f;
    return averageVoltage;
}

float SensorManager::getLocalBatteryPercent() const
{
    float voltage = getLocalBatteryVoltage();
    float percent = (voltage - 3.2f) / (4.2f - 3.2f) * 100.0f;
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;
    return percent;
}

String SensorManager::getLocalSensorDataJSON() const
{
    String json = "{";
    String localIP = WiFi.localIP().toString();
    json += "\"ip\":\"" + localIP + "\",";
    extern int clientId;
    json += "\"clientId\":" + String(clientId) + ",";
    int touchValue = getLocalTouchValue();
    float batteryVoltage = getLocalBatteryVoltage();
    float batteryPercent = getLocalBatteryPercent();
    json += "\"touch\":" + String(touchValue) + ",";
    json += "\"batteryVoltage\":" + String(batteryVoltage, 2) + ",";
    json += "\"batteryPercent\":" + String(batteryPercent, 1);
    json += "}";
    return json;
}

void SensorManager::begin()
{
    pinMode(TOUCH_PIN, INPUT);
    pinMode(BATTERY_PIN, INPUT);
}