#include "web_handlers.h"

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
    html += "<br><a href='/'>← Back to Main Page</a>";
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
}