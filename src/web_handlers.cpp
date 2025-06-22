#include "web_handlers.h"
#include <Update.h>

WebHandlers::WebHandlers(WebServer *webServer, SensorManager *sensorMgr, LEDController *ledCtrl)
    : server(webServer), sensorManager(sensorMgr), ledController(ledCtrl)
{
}

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

void WebHandlers::sendFileInChunks(File &file, String filename)
{
    size_t fileSize = file.size();
    String contentType = getContentType(filename);
    server->sendHeader("Content-Length", String(fileSize));
    server->setContentLength(fileSize);
    server->send(200, contentType, "");

    const size_t bufSize = 1024;
    uint8_t buf[bufSize];
    size_t totalSent = 0;

    while (totalSent < fileSize)
    {
        size_t toRead = min(bufSize, fileSize - totalSent);
        size_t bytesRead = file.read(buf, toRead);
        if (bytesRead == 0)
        {
            Serial.println("Error: Failed to read file");
            break;
        }
        server->sendContent((char *)buf, bytesRead);
        totalSent += bytesRead;
    }

    Serial.printf("File sent successfully. Total bytes: %d\n", totalSent);
}

void WebHandlers::handleRoot()
{
    if (!SPIFFS.exists("/index.html"))
    {
        Serial.println("Error: index.html not found in SPIFFS");
        server->send(500, "text/plain", "File not found in SPIFFS");
        return;
    }

    File file = SPIFFS.open("/index.html", "r");
    if (!file)
    {
        Serial.println("Error: Failed to open index.html");
        server->send(500, "text/plain", "Failed to open file");
        return;
    }

    if (file.size() == 0)
    {
        Serial.println("Error: index.html is empty");
        server->send(500, "text/plain", "File is empty");
        file.close();
        return;
    }

    sendFileInChunks(file, "/index.html");
    file.close();
}

void WebHandlers::handleSensorData()
{
    String senderIP = server->client().remoteIP().toString();
    String clientId = server->arg("clientId");
    int sensorValue = server->arg("value").toInt();

    sensorManager->updateSensorData(senderIP, clientId, sensorValue);
    ledController->setSensorIndicator(sensorValue);

    server->send(200, "text/plain", "OK");
}

void WebHandlers::handleGetSensorData()
{
    String json = sensorManager->getSensorDataJSON();
    server->send(200, "application/json", json);
}

void WebHandlers::handleColor()
{
    int red = server->arg("r").toInt();
    int green = server->arg("g").toInt();
    int blue = server->arg("b").toInt();

    ledController->setColor(red, green, blue);
    server->send(200, "text/plain", "OK");
}

void WebHandlers::handleFileUpload()
{
    HTTPUpload &upload = server->upload();
    static File fsUploadFile;
    static String lastFilename;
    static size_t lastTotalSize = 0;
    static bool uploadSuccess = false;

    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if (!filename.startsWith("/"))
            filename = "/" + filename;
        Serial.printf("handleFileUpload Start: %s\n", filename.c_str());
        fsUploadFile = SPIFFS.open(filename, "w");
        lastFilename = filename;
        lastTotalSize = 0;
        uploadSuccess = fsUploadFile;
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (fsUploadFile)
        {
            fsUploadFile.write(upload.buf, upload.currentSize);
            lastTotalSize += upload.currentSize;
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (fsUploadFile)
        {
            fsUploadFile.close();
            Serial.printf("handleFileUpload Success: %u bytes\n", upload.totalSize);
            uploadSuccess = true;
        }
        // Respond with JSON for AJAX
        String json = String("{\"success\":") + (uploadSuccess ? "true" : "false") + ",\"message\":\"" + (uploadSuccess ? "Upload complete." : "Upload failed.") + "\"}";
        server->send(200, "application/json", json);
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        if (fsUploadFile)
        {
            fsUploadFile.close();
        }
        uploadSuccess = false;
        String json = "{\"success\":false,\"message\":\"Upload aborted.\"}";
        server->send(500, "application/json", json);
    }
}

void WebHandlers::handleDeleteFile()
{
    String filename = server->arg("file");

    if (filename.length() == 0)
    {
        server->send(400, "text/plain", "ERROR: No filename specified");
        Serial.println("Delete request failed: No filename specified");
        return;
    }

    // Ensure filename starts with /
    if (!filename.startsWith("/"))
    {
        filename = "/" + filename;
    }

    Serial.printf("Attempting to delete file: %s\n", filename.c_str());

    // Check if file exists first
    if (!SPIFFS.exists(filename))
    {
        String errorMsg = "ERROR: File '" + filename + "' not found in SPIFFS";
        server->send(404, "text/plain", errorMsg);
        Serial.println(errorMsg);
        return;
    }

    // Attempt to delete the file
    if (SPIFFS.remove(filename))
    {
        String successMsg = "SUCCESS: File '" + filename + "' deleted successfully";
        server->send(200, "text/plain", successMsg);
        Serial.println(successMsg);
    }
    else
    {
        String errorMsg = "ERROR: Failed to delete file '" + filename + "'";
        server->send(500, "text/plain", errorMsg);
        Serial.println(errorMsg);
    }
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
        {
            json += ",";
        }

        json += "{";
        json += "\"name\":\"" + String(file.name()) + "\",";
        json += "\"size\":" + String(file.size());
        json += "}";

        first = false;
        file = root.openNextFile();
    }

    json += "]";
    root.close();

    server->send(200, "application/json", json);
}

void WebHandlers::handleFirmware()
{
    if (!SPIFFS.exists("/firmware_update.html"))
    {
        Serial.println("Error: firmware_update.html not found in SPIFFS");
        server->send(500, "text/plain", "firmware_update.html not found in SPIFFS");
        return;
    }

    File file = SPIFFS.open("/firmware_update.html", "r");
    if (!file)
    {
        Serial.println("Error: Failed to open firmware_update.html");
        server->send(500, "text/plain", "Failed to open firmware_update.html");
        return;
    }

    if (file.size() == 0)
    {
        Serial.println("Error: firmware_update.html is empty");
        server->send(500, "text/plain", "firmware_update.html is empty");
        file.close();
        return;
    }

    sendFileInChunks(file, "/firmware_update.html");
    file.close();
}

void WebHandlers::handleFirmwareUpdate()
{
    String filename = server->arg("file");

    if (filename.length() == 0)
    {
        server->send(400, "text/plain", "ERROR: No firmware file specified");
        Serial.println("Firmware update failed: No filename specified");
        return;
    }

    // Ensure filename starts with /
    if (!filename.startsWith("/"))
    {
        filename = "/" + filename;
    }

    // Check if it's a .bin file
    if (!filename.endsWith(".bin"))
    {
        server->send(400, "text/plain", "ERROR: File must be a .bin firmware file");
        Serial.println("Firmware update failed: Not a .bin file");
        return;
    }

    Serial.printf("Attempting firmware update with file: %s\n", filename.c_str());

    // Check if file exists
    if (!SPIFFS.exists(filename))
    {
        String errorMsg = "ERROR: Firmware file '" + filename + "' not found in SPIFFS";
        server->send(404, "text/plain", errorMsg);
        Serial.println(errorMsg);
        return;
    }

    // Open the firmware file
    File firmwareFile = SPIFFS.open(filename, "r");
    if (!firmwareFile)
    {
        String errorMsg = "ERROR: Cannot open firmware file '" + filename + "'";
        server->send(500, "text/plain", errorMsg);
        Serial.println(errorMsg);
        firmwareFile.close();
        return;
    }

    size_t fileSize = firmwareFile.size();
    Serial.printf("Firmware file size: %d bytes\n", fileSize);

    // Start firmware update
    if (!Update.begin(fileSize))
    {
        String errorMsg = "ERROR: Cannot begin firmware update - " + String(Update.getError());
        server->send(500, "text/plain", errorMsg);
        Serial.println(errorMsg);
        firmwareFile.close();
        return;
    }

    // Send initial response
    server->send(200, "text/plain", "SUCCESS: Firmware update started...");

    // Perform the update
    size_t written = Update.writeStream(firmwareFile);
    firmwareFile.close();

    if (written == fileSize)
    {
        Serial.println("Firmware update written successfully");
        if (Update.end(true))
        {
            Serial.println("Firmware update completed successfully");
            Serial.println("Restarting device...");
            delay(1000);
            ESP.restart();
        }
        else
        {
            Serial.printf("Firmware update failed to end: %s\n", Update.errorString());
        }
    }
    else
    {
        Serial.printf("Firmware update failed: only %d of %d bytes written\n", written, fileSize);
        Update.abort();
    }
}

void WebHandlers::handleUpload()
{
    if (!SPIFFS.exists("/file_manager.html"))
    {
        Serial.println("Error: file_manager.html not found in SPIFFS");
        server->send(500, "text/plain", "file_manager.html not found in SPIFFS");
        return;
    }

    File file = SPIFFS.open("/file_manager.html", "r");
    if (!file)
    {
        Serial.println("Error: Failed to open file_manager.html");
        server->send(500, "text/plain", "Failed to open file_manager.html");
        return;
    }

    if (file.size() == 0)
    {
        Serial.println("Error: file_manager.html is empty");
        server->send(500, "text/plain", "file_manager.html is empty");
        file.close();
        return;
    }

    sendFileInChunks(file, "/file_manager.html");
    file.close();
}

void WebHandlers::handleStaticFile()
{
    String path = server->uri();
    Serial.printf("Handling static file request: %s\n", path.c_str());

    if (!SPIFFS.exists(path))
    {
        Serial.printf("File %s not found\n", path.c_str());
        server->send(404, "text/plain", "File not found");
        return;
    }

    File file = SPIFFS.open(path, "r");
    if (!file)
    {
        Serial.printf("Failed to open file: %s\n", path.c_str());
        server->send(500, "text/plain", "Failed to open file");
        return;
    }

    sendFileInChunks(file, path);
    file.close();
}

void WebHandlers::setupRoutes()
{
    server->on("/", HTTP_GET, [this]()
               { this->handleRoot(); });
    server->on("/sensor", HTTP_POST, [this]()
               { this->handleSensorData(); });
    server->on("/sensorData", HTTP_GET, [this]()
               { this->handleGetSensorData(); });
    server->on("/color", HTTP_GET, [this]()
               { this->handleColor(); });
    server->on("/upload", HTTP_GET, [this]()
               { this->handleUpload(); });
    server->on("/upload", HTTP_POST, []() {}, [this]()
               { this->handleFileUpload(); });
    server->on("/delete", HTTP_POST, [this]()
               { this->handleDeleteFile(); });
    server->on("/list", HTTP_GET, [this]()
               { this->handleListFiles(); });
    server->on("/firmware", HTTP_GET, [this]()
               { this->handleFirmware(); });
    server->on("/firmwareUpdate", HTTP_POST, [this]()
               { this->handleFirmwareUpdate(); });

    // Add static file handler
    server->onNotFound([this]()
                       {
        String path = server->uri();
        if (path.endsWith(".css") || path.endsWith(".js") || path.endsWith(".html"))
        {
            this->handleStaticFile();
        }
        else
        {
            server->send(404, "text/plain", "Not found");
        } });
}