#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <WebServer.h>
#include <SPIFFS.h>
#include <Update.h>
#include "sensor_manager.h"
#include "led_controller.h"

class WebHandlers
{
private:
    WebServer *server;
    SensorManager *sensorManager;
    LEDController *ledController;

    void sendFileInChunks(File &file);

public:
    WebHandlers(WebServer *webServer, SensorManager *sensorMgr, LEDController *ledCtrl);

    void handleRoot();
    void handleSensorData();
    void handleGetSensorData();
    void handleColor();
    void handleFileUpload();
    void handleUpload();
    void handleFirmwareUpload();
    void handleFirmwareUpdate();
    void handleDeleteFile();

    void setupRoutes();
};

#endif // WEB_HANDLERS_H