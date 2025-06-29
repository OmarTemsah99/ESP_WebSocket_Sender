#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <ArduinoOTA.h>

class WiFiManager
{
private:
    unsigned long lastReconnectAttempt;

    void setupOTA();
    void printWiFiStatus();

public:
    WiFiManager();

    bool init();
    void handleConnection();
    bool isConnected();
    void printConnectionInfo();
};

#endif // WIFI_MANAGER_H