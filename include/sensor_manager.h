#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <map>
#include <string>
#include <Arduino.h>

struct SensorData
{
    String clientId;
    int value;
};

class SensorManager
{
private:
    std::map<String, SensorData> sensorDataMap;

public:
    void updateSensorData(const String &senderIP, const String &clientId, int sensorValue);
    String getSensorDataJSON() const;
    const std::map<String, SensorData> &getAllSensorData() const;
    void clearSensorData();
    bool hasSensorData() const;
};

#endif // SENSOR_MANAGER_H