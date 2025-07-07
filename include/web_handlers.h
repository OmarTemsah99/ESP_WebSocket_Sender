// ==================== FIXED web_handlers.h ====================
#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <WebServer.h>
#include <SPIFFS.h>
#include "sensor_manager.h"
#include "client_config.h"

class WebHandlers
{
private:
    WebServer *server;
    SensorManager *sensorManager;
    ClientConfig *clientConfig;

    String getContentType(String filename);
    bool sendFile(String path);
    bool isValidFileExtension(String filename);
    void sendJsonResponse(bool success, String message = "", String data = "");

public:
    WebHandlers(WebServer *webServer, SensorManager *sensorMgr, ClientConfig *clientCfg);

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