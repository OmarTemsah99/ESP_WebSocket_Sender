#include "wifi_manager.h"
#include "config.h"
#include <SPIFFS.h>
#include <Update.h>

WiFiManager::WiFiManager(LEDController *ledCtrl) : ledController(ledCtrl)
{
    lastReconnectAttempt = 0;
}

void WiFiManager::setupOTA()
{
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]()
                       {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        if (type == "filesystem") {
            SPIFFS.end();
        }
        Serial.println("Start updating " + type); });

    ArduinoOTA.onEnd([]()
                     {
        Serial.println("\nOTA Update completed");
        if (!SPIFFS.begin(true)) {
            Serial.println("SPIFFS remount failed");
        } });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });

    ArduinoOTA.onError([](ota_error_t error)
                       {
        Serial.printf("Error[%u]: ", error);
        switch (error) {
            case OTA_AUTH_ERROR: Serial.println("Auth Failed"); break;
            case OTA_BEGIN_ERROR: Serial.println("Begin Failed"); break;
            case OTA_CONNECT_ERROR: Serial.println("Connect Failed"); break;
            case OTA_RECEIVE_ERROR: Serial.println("Receive Failed"); break;
            case OTA_END_ERROR: Serial.println("End Failed"); break;
        } });

    ArduinoOTA.begin();
    Serial.println("OTA Ready");
}

void WiFiManager::printWiFiStatus()
{
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

bool WiFiManager::init()
{
    WiFi.mode(WIFI_STA);

    Serial.println("Configuring Static IP...");
    if (!WiFi.config(STATIC_IP, GATEWAY, SUBNET, DNS_SERVER))
    {
        Serial.println("Static IP Configuration Failed!");
    }
    else
    {
        Serial.println("Static IP Configuration Successful");
    }

    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int connectionAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && connectionAttempts < CONNECTION_TIMEOUT)
    {
        ledController->setConnectingIndicator();
        delay(500);
        Serial.print(".");
        connectionAttempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        ledController->setConnectedIndicator();
        printConnectionInfo();
        setupOTA();
        return true;
    }
    else
    {
        ledController->setDisconnectedIndicator();
        Serial.println("Failed to connect to WiFi!");
        printWiFiStatus();
        return false;
    }
}

void WiFiManager::handleConnection()
{
    ArduinoOTA.handle();

    if (WiFi.status() != WL_CONNECTED)
    {
        unsigned long currentMillis = millis();
        if (currentMillis - lastReconnectAttempt > RECONNECT_INTERVAL)
        {
            Serial.println("WiFi connection lost! Reconnecting...");
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            lastReconnectAttempt = currentMillis;
        }
        ledController->setConnectingIndicator();
    }
}

bool WiFiManager::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::printConnectionInfo()
{
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
}