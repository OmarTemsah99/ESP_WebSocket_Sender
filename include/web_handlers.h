#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <WebServer.h>
#include <SPIFFS.h>
#include "sensor_manager.h"

class WebHandlers
{
private:
    WebServer *server;
    SensorManager *sensorManager;
    int *clientIdPtr; // Store pointer to clientId for cleaner access

    // Helper methods
    String getContentType(String filename);
    bool sendFile(String path);
    bool isValidFileExtension(String filename);
    void sendJsonResponse(bool success, String message = "", String data = "");

public:
    WebHandlers(WebServer *webServer, SensorManager *sensorMgr);

    // Core functionality
    void setupRoutes(int &clientId);

    // Route handlers - grouped by functionality
    void handleRoot();
    void handleStaticFile();

    // Sensor data handlers
    void handleSensorData();
    void handleGetSensorData();
    void handleGetLocalSensorData();
    void handleSensorDataPage();
    void handleSetClientId();

    // File management handlers
    void handleUpload();
    void handleFileUpload();
    void handleDeleteFile();
    void handleListFiles();

    // Firmware handlers
    void handleFirmware();
    void handleFirmwareUpdate();
};

#endif