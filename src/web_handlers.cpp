#include "web_handlers.h"
#include <Update.h>

WebHandlers::WebHandlers(WebServer *webServer, SensorManager *sensorMgr, LEDController *ledCtrl)
    : server(webServer), sensorManager(sensorMgr), ledController(ledCtrl)
{
}

void WebHandlers::sendFileInChunks(File &file)
{
    size_t fileSize = file.size();
    server->sendHeader("Content-Length", String(fileSize));
    server->setContentLength(fileSize);
    server->send(200, "text/html", "");

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

    sendFileInChunks(file);
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

    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if (!filename.startsWith("/"))
            filename = "/" + filename;
        Serial.printf("handleFileUpload Start: %s\n", filename.c_str());
        fsUploadFile = SPIFFS.open(filename, "w");
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (fsUploadFile)
        {
            fsUploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (fsUploadFile)
        {
            fsUploadFile.close();
            Serial.printf("handleFileUpload Success: %u bytes\n", upload.totalSize);
        }
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
    String html = "<!DOCTYPE html><html><head><title>Firmware Update</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial; margin: 20px; background-color: #f5f5f5; }";
    html += ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += "h2 { color: #2c3e50; border-bottom: 2px solid #dc3545; padding-bottom: 10px; }";
    html += ".warning { background: #f8d7da; color: #721c24; padding: 15px; border-radius: 5px; margin-bottom: 20px; border: 1px solid #f5c6cb; }";
    html += ".firmware-form { background: #fff3cd; padding: 15px; border-radius: 5px; margin-bottom: 20px; border: 1px solid #ffeaa7; }";
    html += ".file-list { background: #fff; border: 1px solid #ddd; border-radius: 5px; margin-bottom: 20px; }";
    html += ".file-item { padding: 10px; border-bottom: 1px solid #eee; display: flex; justify-content: space-between; align-items: center; }";
    html += ".file-item:last-child { border-bottom: none; }";
    html += ".file-name { font-weight: bold; color: #2c3e50; }";
    html += ".file-size { color: #7f8c8d; font-size: 0.9em; }";
    html += ".update-btn { background: #dc3545; color: white; border: none; padding: 5px 15px; border-radius: 3px; cursor: pointer; }";
    html += ".update-btn:hover { background: #c82333; }";
    html += "input[type='file'] { margin: 10px 0; width: 100%; }";
    html += "input[type='submit'] { background: #dc3545; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; }";
    html += "input[type='submit']:hover { background: #c82333; }";
    html += ".refresh-btn { background: #6c757d; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; margin-bottom: 10px; }";
    html += ".refresh-btn:hover { background: #5a6268; }";
    html += ".back-btn { background: #28a745; color: white; text-decoration: none; padding: 10px 20px; border-radius: 5px; display: inline-block; margin-top: 20px; }";
    html += ".back-btn:hover { background: #218838; }";
    html += ".progress { width: 100%; background-color: #f0f0f0; border-radius: 5px; margin: 10px 0; }";
    html += ".progress-bar { height: 20px; background-color: #28a745; border-radius: 5px; width: 0%; transition: width 0.3s; }";
    html += "#uploadStatus { margin: 10px 0; font-weight: bold; }";
    html += "</style>";
    html += "<script>";
    html += "function updateFromFile(filename) {";
    html += "  if(confirm('WARNING: This will update the firmware and restart the device. Continue?')) {";
    html += "    document.getElementById('uploadStatus').innerHTML = 'Starting firmware update...';";
    html += "    document.getElementById('uploadStatus').style.color = 'orange';";
    html += "    fetch('/firmwareUpdate?file=' + encodeURIComponent(filename), { method: 'POST' })";
    html += "    .then(response => response.text())";
    html += "    .then(data => {";
    html += "      document.getElementById('uploadStatus').innerHTML = data;";
    html += "      if(data.includes('SUCCESS')) {";
    html += "        document.getElementById('uploadStatus').style.color = 'green';";
    html += "        setTimeout(() => { alert('Device will restart now. Please reconnect after 30 seconds.'); }, 2000);";
    html += "      } else {";
    html += "        document.getElementById('uploadStatus').style.color = 'red';";
    html += "      }";
    html += "    })";
    html += "    .catch(error => {";
    html += "      document.getElementById('uploadStatus').innerHTML = 'Error: ' + error;";
    html += "      document.getElementById('uploadStatus').style.color = 'red';";
    html += "    });";
    html += "  }";
    html += "}";
    html += "function refreshFiles() { location.reload(); }";
    html += "</script>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h2>Firmware Update</h2>";

    html += "<div class='warning'>";
    html += "<strong>WARNING:</strong><br>";
    html += "Firmware updates can brick your device if interrupted<br>";
    html += "Ensure stable power supply during update<br>";
    html += "Only use firmware files specifically for ESP32-S3<br>";
    html += "Device will restart automatically after update<br>";
    html += "Have your WiFi credentials ready for reconnection";
    html += "</div>";

    html += "<div class='firmware-form'>";
    html += "<h3>Upload New Firmware</h3>";
    html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
    html += "<input type='file' name='upload' accept='.bin' required>";
    html += "<input type='submit' value='Upload Firmware (.bin)'>";
    html += "</form>";
    html += "<div id='uploadStatus'></div>";
    html += "</div>";

    html += "<button class='refresh-btn' onclick='refreshFiles()'>Refresh File List</button>";
    html += "<h3>Available Firmware Files (.bin):</h3>";
    html += "<div class='file-list'>";

    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    bool hasBinFiles = false;

    while (file)
    {
        String fileName = file.name();
        if (fileName.endsWith(".bin"))
        {
            hasBinFiles = true;
            size_t fileSize = file.size();

            html += "<div class='file-item'>";
            html += "<div>";
            html += "<span class='file-name'>" + fileName + "</span><br>";
            html += "<span class='file-size'>" + String(fileSize) + " bytes</span>";
            html += "</div>";
            html += "<button class='update-btn' onclick='updateFromFile(\"" + fileName + "\")'>Update Firmware</button>";
            html += "</div>";
        }
        file = root.openNextFile();
    }

    if (!hasBinFiles)
    {
        html += "<div class='file-item'>No .bin firmware files found. Upload a firmware file first.</div>";
    }

    html += "</div>";
    html += "<a href='/' class='back-btn'>Back to Main Page</a>";
    html += "</div>";
    html += "</body></html>";

    server->send(200, "text/html", html);
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
    String html = "<!DOCTYPE html><html><head><title>File Manager</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial; margin: 20px; background-color: #f5f5f5; }";
    html += ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += "h2 { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 10px; }";
    html += ".upload-form { background: #ecf0f1; padding: 15px; border-radius: 5px; margin-bottom: 20px; }";
    html += ".file-list { background: #fff; border: 1px solid #ddd; border-radius: 5px; }";
    html += ".file-item { padding: 10px; border-bottom: 1px solid #eee; display: flex; justify-content: space-between; align-items: center; }";
    html += ".file-item:last-child { border-bottom: none; }";
    html += ".file-name { font-weight: bold; color: #2c3e50; }";
    html += ".file-size { color: #7f8c8d; font-size: 0.9em; }";
    html += ".delete-btn { background: #e74c3c; color: white; border: none; padding: 5px 10px; border-radius: 3px; cursor: pointer; }";
    html += ".delete-btn:hover { background: #c0392b; }";
    html += "input[type='file'] { margin: 10px 0; }";
    html += "input[type='submit'] { background: #3498db; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; }";
    html += "input[type='submit']:hover { background: #2980b9; }";
    html += ".refresh-btn { background: #27ae60; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; margin-bottom: 10px; }";
    html += ".refresh-btn:hover { background: #229954; }";
    html += "</style>";
    html += "<script>";
    html += "function deleteFile(filename) {";
    html += "  if(confirm('Are you sure you want to delete ' + filename + '?')) {";
    html += "    fetch('/delete?file=' + encodeURIComponent(filename), { method: 'POST' })";
    html += "    .then(response => response.text())";
    html += "    .then(data => { alert(data); location.reload(); })";
    html += "    .catch(error => { alert('Error: ' + error); });";
    html += "  }";
    html += "}";
    html += "function refreshFiles() { location.reload(); }";
    html += "</script>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h2>ESP32 File Manager</h2>";

    html += "<div class='upload-form'>";
    html += "<h3>Upload New File</h3>";
    html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
    html += "<input type='file' name='upload' required>";
    html += "<input type='submit' value='Upload File'>";
    html += "</form>";
    html += "</div>";

    html += "<button class='refresh-btn' onclick='refreshFiles()'>Refresh File List</button>";
    html += "<h3>Current Files in SPIFFS:</h3>";
    html += "<div class='file-list'>";

    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    bool hasFiles = false;

    while (file)
    {
        hasFiles = true;
        String fileName = file.name();
        size_t fileSize = file.size();

        html += "<div class='file-item'>";
        html += "<div>";
        html += "<span class='file-name'>" + fileName + "</span><br>";
        html += "<span class='file-size'>" + String(fileSize) + " bytes</span>";
        html += "</div>";
        html += "<button class='delete-btn' onclick='deleteFile(\"" + fileName + "\")'>Delete</button>";
        html += "</div>";

        file = root.openNextFile();
    }

    if (!hasFiles)
    {
        html += "<div class='file-item'>No files found in SPIFFS</div>";
    }

    html += "</div>";
    html += "<br><a href='/'>Back to Main Page</a>";
    html += "</div>";
    html += "</body></html>";

    server->send(200, "text/html", html);
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
    server->on("/upload", HTTP_POST, [this]()
               { server->send(200, "text/plain", "Upload complete - <a href='/upload'>Back to File Manager</a>"); }, [this]()
               { this->handleFileUpload(); });
    server->on("/delete", HTTP_POST, [this]()
               { this->handleDeleteFile(); });
    server->on("/files", HTTP_GET, [this]()
               { this->handleListFiles(); });
    server->on("/firmware", HTTP_GET, [this]()
               { this->handleFirmware(); });
    server->on("/firmwareUpdate", HTTP_POST, [this]()
               { this->handleFirmwareUpdate(); });
}