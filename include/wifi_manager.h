#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <ArduinoOTA.h>
#include "led_controller.h"

class WiFiManager
{
private:
    LEDController *ledController;
    unsigned long lastReconnectAttempt;

    void setupOTA();
    void printWiFiStatus();

public:
    WiFiManager(LEDController *ledCtrl);

    bool init();
    void handleConnection();
    bool isConnected();
    void printConnectionInfo();
};

#endif // WIFI_MANAGER_H