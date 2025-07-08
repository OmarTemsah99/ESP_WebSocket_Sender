// ==================== FIXED web_handlers.h ====================
#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <WebServer.h>
#include <SPIFFS.h>
#include "sensor_manager.h"
#include "ClientIdentity.h"

class WebHandlers
{
private:
    WebServer *server;
    SensorManager *sensorManager;
    ClientIdentity *clientIdentity;

    String getContentType(String filename);
    bool sendFile(String path);
    bool isValidFileExtension(String filename);
    void sendJsonResponse(bool success, String message = "", String data = "");

public:
    WebHandlers(WebServer *webServer, SensorManager *sensorMgr, ClientIdentity *clientIdentity);

    void setupRoutes();

    void handleRoot();
    void handleStaticFile();

    // Sensor
    void handleSensorData();
    void handleGetSensorData();
    void handleGetLocalSensorData();
    void handleSensorDataPage();
    void handleSetClientId();

    // File Management
    void handleUpload();
    void handleFileUpload();
    void handleDeleteFile();
    void handleListFiles();

    // Firmware
    void handleFirmware();
    void handleFirmwareUpdate();
};

#endif