#include "web_handlers.h"
#include <Update.h>
#include "ClientIdentity.h"

WebHandlers::WebHandlers(WebServer *webServer, SensorManager *sensorMgr, ClientIdentity *clientIdentity)
    : server(webServer), sensorManager(sensorMgr), clientIdentity(clientIdentity) {}

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

bool WebHandlers::sendFile(String path)
{
    if (!SPIFFS.exists(path))
    {
        server->send(404, "text/plain", "File not found");
        return false;
    }
    File file = SPIFFS.open(path, "r");
    if (!file)
    {
        server->send(500, "text/plain", "Cannot open file");
        return false;
    }
    server->streamFile(file, getContentType(path));
    file.close();
    return true;
}

bool WebHandlers::isValidFileExtension(String filename)
{
    return filename.endsWith(".html") || filename.endsWith(".css") || filename.endsWith(".js") || filename.endsWith(".bin");
}

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

void WebHandlers::handleRoot() { sendFile("/index.html"); }
void WebHandlers::handleStaticFile() { sendFile(server->uri()); }

void WebHandlers::handleSensorData()
{
    String ip = server->client().remoteIP().toString();
    int touch = server->arg("touch").toInt();
    float voltage = server->arg("batteryVoltage").toFloat();
    float percent = server->arg("batteryPercent").toFloat();
    String clientId = server->arg("clientId");
    sensorManager->updateSensorData(ip, clientId, touch, voltage, percent);
    server->send(200, "text/plain", "OK");
}

void WebHandlers::handleGetSensorData()
{
    server->send(200, "application/json", sensorManager->getSensorDataJSON());
}

void WebHandlers::handleGetLocalSensorData()
{
    server->send(200, "application/json", sensorManager->getLocalSensorDataJSON());
}

void WebHandlers::handleSensorDataPage() { sendFile("/sensor_data.html"); }

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
    clientIdentity->set(newId);
    sendJsonResponse(true, "Client ID updated", "\"clientId\":" + String(newId));
    Serial.printf("[CLIENT_ID] Updated to %d\n", newId);
}

void WebHandlers::handleUpload() { sendFile("/file_manager.html"); }

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
            sendJsonResponse(false, "Invalid file type");
            return;
        }
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
    if (filename.isEmpty())
    {
        sendJsonResponse(false, "No file specified");
        return;
    }
    if (!filename.startsWith("/"))
        filename = "/" + filename;
    bool success = SPIFFS.remove(filename);
    sendJsonResponse(success, success ? "File deleted" : "Delete failed");
}

void WebHandlers::handleListFiles()
{
    String json = "[";
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    bool first = true;
    while (file)
    {
        if (!first)
            json += ",";
        json += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
        first = false;
        file = root.openNextFile();
    }
    json += "]";
    server->send(200, "application/json", json);
}

void WebHandlers::handleFirmware() { sendFile("/firmware_update.html"); }

void WebHandlers::handleFirmwareUpdate()
{
    String filename = server->arg("file");
    if (!filename.startsWith("/"))
        filename = "/" + filename;
    if (!filename.endsWith(".bin"))
    {
        sendJsonResponse(false, "File must be .bin");
        return;
    }
    if (!SPIFFS.exists(filename))
    {
        sendJsonResponse(false, "Firmware file not found");
        return;
    }
    File firmwareFile = SPIFFS.open(filename);
    if (!Update.begin(firmwareFile.size()))
    {
        firmwareFile.close();
        sendJsonResponse(false, "Failed to begin update");
        return;
    }
    server->send(200, "text/plain", "Firmware update started");
    size_t written = Update.writeStream(firmwareFile);
    firmwareFile.close();
    if (written == firmwareFile.size() && Update.end(true))
    {
        ESP.restart();
    }
    else
    {
        Update.abort();
    }
}

void WebHandlers::setupRoutes()
{
    server->on("/", HTTP_GET, [this]()
               { handleRoot(); });
    server->on("/sensorpage", HTTP_GET, [this]()
               { handleSensorDataPage(); });
    server->on("/upload", HTTP_GET, [this]()
               { handleUpload(); });
    server->on("/firmware", HTTP_GET, [this]()
               { handleFirmware(); });
    server->on("/sensor", HTTP_POST, [this]()
               { handleSensorData(); });
    server->on("/sensorData", HTTP_GET, [this]()
               { handleGetSensorData(); });
    server->on("/localSensorData", HTTP_GET, [this]()
               { handleGetLocalSensorData(); });
    server->on("/setClientId", HTTP_POST, [this]()
               { handleSetClientId(); });

    server->on("/getClientId", HTTP_GET, [this]()
               {
    int id = clientIdentity->get();
    server->send(200, "application/json", "{\"clientId\":" + String(id) + "}"); });

    server->on("/upload", HTTP_POST, []() {}, [this]()
               { handleFileUpload(); });
    server->on("/delete", HTTP_POST, [this]()
               { handleDeleteFile(); });
    server->on("/list", HTTP_GET, [this]()
               { handleListFiles(); });
    server->on("/firmwareUpdate", HTTP_POST, [this]()
               { handleFirmwareUpdate(); });
    server->onNotFound([this]()
                       {
        String path = server->uri();
        if (path.endsWith(".html") || path.endsWith(".css") || path.endsWith(".js")) {
            handleStaticFile();
        } else {
            server->send(404, "text/plain", "Not found");
        } });
}
