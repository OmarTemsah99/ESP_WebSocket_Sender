#include "web_handlers.h"
#include <Update.h>

WebHandlers::WebHandlers(WebServer *webServer, SensorManager *sensorMgr)
    : server(webServer), sensorManager(sensorMgr), clientIdPtr(nullptr)
{
}

// Helper method to determine content type
String WebHandlers::getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    if (filename.endsWith(".css"))
        return "text/css";
    if (filename.endsWith(".js"))
        return "application/javascript";
    if (filename.endsWith(".json"))
        return "application/json";
    return "text/plain";
}

// Simplified file sending method
bool WebHandlers::sendFile(String path)
{
    if (!SPIFFS.exists(path))
    {
        Serial.printf("File not found: %s\n", path.c_str());
        server->send(404, "text/plain", "File not found");
        return false;
    }

    File file = SPIFFS.open(path, "r");
    if (!file || file.size() == 0)
    {
        Serial.printf("Cannot open or empty file: %s\n", path.c_str());
        server->send(500, "text/plain", "Cannot open file");
        file.close();
        return false;
    }

    // Stream file directly (ESP32 WebServer handles chunking automatically)
    server->streamFile(file, getContentType(path));
    file.close();
    return true;
}

// Check if file extension is allowed
bool WebHandlers::isValidFileExtension(String filename)
{
    return filename.endsWith(".html") ||
           filename.endsWith(".css") ||
           filename.endsWith(".js") ||
           filename.endsWith(".bin");
}

// Simplified JSON response helper
void WebHandlers::sendJsonResponse(bool success, String message, String data)
{
    String json = "{\"success\":" + String(success ? "true" : "false");
    if (message.length() > 0)
        json += ",\"message\":\"" + message + "\"";
    if (data.length() > 0)
        json += "," + data;
    json += "}";
    server->send(success ? 200 : 400, "application/json", json);
}

// ========================= ROUTE HANDLERS =========================

void WebHandlers::handleRoot()
{
    sendFile("/index.html");
}

void WebHandlers::handleStaticFile()
{
    sendFile(server->uri());
}

void WebHandlers::handleSensorData()
{
    String senderIP = server->client().remoteIP().toString();
    String clientId = server->arg("clientId");
    int sensorValue = server->arg("value").toInt();

    sensorManager->updateSensorData(senderIP, clientId, sensorValue);
    server->send(200, "text/plain", "OK");
}

void WebHandlers::handleGetSensorData()
{
    String json = sensorManager->getSensorDataJSON();
    server->send(200, "application/json", json);
}

void WebHandlers::handleGetLocalSensorData()
{
    String json = sensorManager->getLocalSensorDataJSON();
    server->send(200, "application/json", json);
}

void WebHandlers::handleSensorDataPage()
{
    sendFile("/sensor_data.html");
}

void WebHandlers::handleSetClientId()
{
    if (!server->hasArg("id"))
    {
        sendJsonResponse(false, "Missing ID parameter");
        return;
    }

    int newId = server->arg("id").toInt();
    if (newId < 0 || newId > 15)
    {
        sendJsonResponse(false, "ID must be between 0-15");
        return;
    }

    *clientIdPtr = newId;
    sendJsonResponse(true, "Client ID updated", "\"clientId\":" + String(newId));
    Serial.printf("[CLIENT_ID] Updated to %d\n", newId);
}

void WebHandlers::handleUpload()
{
    sendFile("/file_manager.html");
}

void WebHandlers::handleFileUpload()
{
    HTTPUpload &upload = server->upload();
    static File uploadFile;
    static bool uploadSuccess = false;

    switch (upload.status)
    {
    case UPLOAD_FILE_START:
    {
        String filename = upload.filename;
        if (!filename.startsWith("/"))
            filename = "/" + filename;

        if (!isValidFileExtension(filename))
        {
            Serial.printf("Rejected: %s (invalid extension)\n", filename.c_str());
            sendJsonResponse(false, "Only .html, .css, .js, .bin files allowed");
            return;
        }

        Serial.printf("Upload started: %s\n", filename.c_str());
        uploadFile = SPIFFS.open(filename, "w");
        uploadSuccess = uploadFile;
        break;
    }

    case UPLOAD_FILE_WRITE:
        if (uploadFile)
            uploadFile.write(upload.buf, upload.currentSize);
        break;

    case UPLOAD_FILE_END:
        if (uploadFile)
            uploadFile.close();
        sendJsonResponse(uploadSuccess, uploadSuccess ? "Upload complete" : "Upload failed");
        Serial.printf("Upload %s: %u bytes\n", uploadSuccess ? "success" : "failed", upload.totalSize);
        break;

    case UPLOAD_FILE_ABORTED:
        if (uploadFile)
            uploadFile.close();
        sendJsonResponse(false, "Upload aborted");
        break;
    }
}

void WebHandlers::handleDeleteFile()
{
    String filename = server->arg("file");
    if (filename.length() == 0)
    {
        sendJsonResponse(false, "No filename specified");
        return;
    }

    if (!filename.startsWith("/"))
        filename = "/" + filename;

    if (!SPIFFS.exists(filename))
    {
        sendJsonResponse(false, "File not found: " + filename);
        return;
    }

    bool success = SPIFFS.remove(filename);
    sendJsonResponse(success, success ? "File deleted" : "Delete failed");
    Serial.printf("Delete %s: %s\n", filename.c_str(), success ? "success" : "failed");
}

void WebHandlers::handleListFiles()
{
    String json = "[";
    bool first = true;

    File root = SPIFFS.open("/");
    File file = root.openNextFile();

    while (file)
    {
        if (!first)
            json += ",";
        json += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
        first = false;
        file = root.openNextFile();
    }

    json += "]";
    root.close();
    server->send(200, "application/json", json);
}

void WebHandlers::handleFirmware()
{
    sendFile("/firmware_update.html");
}

void WebHandlers::handleFirmwareUpdate()
{
    String filename = server->arg("file");
    if (filename.length() == 0)
    {
        sendJsonResponse(false, "No firmware file specified");
        return;
    }

    if (!filename.startsWith("/"))
        filename = "/" + filename;

    if (!filename.endsWith(".bin"))
    {
        sendJsonResponse(false, "File must be a .bin firmware file");
        return;
    }

    if (!SPIFFS.exists(filename))
    {
        sendJsonResponse(false, "Firmware file not found: " + filename);
        return;
    }

    File firmwareFile = SPIFFS.open(filename, "r");
    if (!firmwareFile)
    {
        sendJsonResponse(false, "Cannot open firmware file");
        return;
    }

    size_t fileSize = firmwareFile.size();
    Serial.printf("Starting firmware update: %s (%d bytes)\n", filename.c_str(), fileSize);

    if (!Update.begin(fileSize))
    {
        firmwareFile.close();
        sendJsonResponse(false, "Cannot begin firmware update");
        return;
    }

    server->send(200, "text/plain", "Firmware update started...");

    size_t written = Update.writeStream(firmwareFile);
    firmwareFile.close();

    if (written == fileSize && Update.end(true))
    {
        Serial.println("Firmware update successful. Restarting...");
        delay(1000);
        ESP.restart();
    }
    else
    {
        Serial.printf("Firmware update failed: %d/%d bytes written\n", written, fileSize);
        Update.abort();
    }
}

// ========================= SETUP ROUTES =========================

void WebHandlers::setupRoutes(int &clientId)
{
    clientIdPtr = &clientId; // Store reference for use in handlers

    // Main routes
    server->on("/", HTTP_GET, [this]()
               { handleRoot(); });
    server->on("/sensorpage", HTTP_GET, [this]()
               { handleSensorDataPage(); });
    server->on("/upload", HTTP_GET, [this]()
               { handleUpload(); });
    server->on("/firmware", HTTP_GET, [this]()
               { handleFirmware(); });

    // API routes
    server->on("/sensor", HTTP_POST, [this]()
               { handleSensorData(); });
    server->on("/sensorData", HTTP_GET, [this]()
               { handleGetSensorData(); });
    server->on("/localSensorData", HTTP_GET, [this]()
               { handleGetLocalSensorData(); });
    server->on("/setClientId", HTTP_POST, [this]()
               { handleSetClientId(); });

    // File management
    server->on("/upload", HTTP_POST, []() {}, [this]()
               { handleFileUpload(); });
    server->on("/delete", HTTP_POST, [this]()
               { handleDeleteFile(); });
    server->on("/list", HTTP_GET, [this]()
               { handleListFiles(); });
    server->on("/firmwareUpdate", HTTP_POST, [this]()
               { handleFirmwareUpdate(); });

    // Static file handler
    server->onNotFound([this]()
                       {
        String path = server->uri();
        if (path.endsWith(".css") || path.endsWith(".js") || path.endsWith(".html"))
        {
            handleStaticFile();
        }
        else
        {
            server->send(404, "text/plain", "Not found");
        } });
}