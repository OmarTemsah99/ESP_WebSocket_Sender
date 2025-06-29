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

    String getContentType(String filename);
    void sendFileInChunks(File &file, String filename);
    void handleStaticFile();

public:
    WebHandlers(WebServer *webServer, SensorManager *sensorMgr);

    void handleRoot();
    void handleSensorData();
    void handleGetSensorData();
    void handleGetLocalSensorData();       // New method for local sensor data
    void handleSetClientId(int &clientId); // New method for setting client ID
    void handleFileUpload();
    void handleUpload();
    void handleDeleteFile();
    void handleListFiles();
    void handleFirmware();
    void handleFirmwareUpdate();
    void handleSensorDataPage();     // Serve the dedicated sensor data page
    void setupRoutes(int &clientId); // Pass clientId reference for /setClientId
};

#endif