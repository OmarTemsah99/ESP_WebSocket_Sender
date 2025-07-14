#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <ESPAsyncWebServer.h> // Changed from WebServer.h
#include <AsyncTCP.h>          // Required for ESPAsyncWebServer
#include <SPIFFS.h>
#include "sensor_manager.h"

class ClientIdentity; // Forward declaration

class WebHandlers
{
private:
    AsyncWebServer *server; // Changed from WebServer
    SensorManager *sensorManager;
    ClientIdentity *clientIdentity;

    // Helper methods
    String getContentType(String filename);
    bool sendFile(String path, AsyncWebServerRequest *request);
    bool isValidFileExtension(String filename);
    void sendJsonResponse(AsyncWebServerRequest *request, bool success, String message = "", String data = "");

public:
    WebHandlers(AsyncWebServer *webServer, SensorManager *sensorMgr, ClientIdentity *clientIdentity);
    void setupRoutes();

    // Route handlers
    void handleRoot(AsyncWebServerRequest *request);
    void handleStaticFile(AsyncWebServerRequest *request);
    void handleSensorData(AsyncWebServerRequest *request);
    void handleGetSensorData(AsyncWebServerRequest *request);
    void handleGetLocalSensorData(AsyncWebServerRequest *request);
    void handleSensorDataPage(AsyncWebServerRequest *request);
    void handleSetClientId(AsyncWebServerRequest *request);
    void handleUpload(AsyncWebServerRequest *request);
    void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleDeleteFile(AsyncWebServerRequest *request);
    void handleListFiles(AsyncWebServerRequest *request);
    void handleFirmware(AsyncWebServerRequest *request);
    void handleFirmwareUpdate(AsyncWebServerRequest *request);
};

#endif