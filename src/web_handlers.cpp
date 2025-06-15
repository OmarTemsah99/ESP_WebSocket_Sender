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

void WebHandlers::handleFirmwareUpload()
{
    HTTPUpload &upload = server->upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        Serial.printf("Firmware Upload Start: %s\n", upload.filename.c_str());
        Serial.setDebugOutput(true);

        // Check if filename has .bin extension
        if (!upload.filename.endsWith(".bin"))
        {
            Serial.println("Error: File must have .bin extension");
            return;
        }

        // Set LED to indicate firmware update in progress
        ledController->setColor(255, 255, 0); // Yellow for update in progress

        // Calculate sketch space for firmware update
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace, U_FLASH))
        {
            Serial.println("Error: Cannot start firmware update");
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        Serial.printf("Firmware Upload Progress: %u bytes\n", upload.totalSize);

        // Write firmware data
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
            Serial.println("Error: Failed to write firmware data");
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (Update.end(true))
        {
            Serial.printf("Firmware Update Success: %u bytes\n", upload.totalSize);
            Serial.println("Rebooting...");

            // Set LED to indicate successful update
            ledController->setColor(0, 255, 0); // Green for success
            delay(1000);
        }
        else
        {
            Serial.println("Firmware Update Failed");
            Update.printError(Serial);

            // Set LED to indicate failed update
            ledController->setColor(255, 0, 0); // Red for failure
        }
        Serial.setDebugOutput(false);
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        Serial.println("Firmware upload aborted");
        Update.end();

        // Set LED to indicate aborted update
        ledController->setColor(255, 0, 0); // Red for failure
    }
}

void WebHandlers::handleFirmwareUpdate()
{
    String html = "<html><head><title>Firmware Update</title>";
    html += "<style>";
    html += "body { font-family: Arial; text-align: center; margin: 20px; background-color: #f0f0f0; }";
    html += ".container { max-width: 600px; margin: 0 auto; padding: 20px; background-color: white; border-radius: 10px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
    html += ".warning { background-color: #fff3cd; border: 1px solid #ffeaa7; color: #856404; padding: 15px; margin: 15px 0; border-radius: 5px; }";
    html += ".info { background-color: #d1ecf1; border: 1px solid #bee5eb; color: #0c5460; padding: 15px; margin: 15px 0; border-radius: 5px; }";
    html += "input[type='file'] { margin: 10px; padding: 5px; }";
    html += "input[type='submit'] { background-color: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }";
    html += "input[type='submit']:hover { background-color: #0056b3; }";
    html += ".back-btn { background-color: #6c757d; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; text-decoration: none; display: inline-block; margin: 10px; }";
    html += ".back-btn:hover { background-color: #545b62; }";
    html += "</style></head><body>";

    html += "<div class='container'>";
    html += "<h1>ESP32 Firmware Update</h1>";

    html += "<div class='warning'>";
    html += "<strong>⚠️ Warning!</strong><br>";
    html += "• Only upload .bin firmware files<br>";
    html += "• Do not power off during update<br>";
    html += "• Device will reboot after successful update<br>";
    html += "• Make sure the firmware is compatible with your ESP32-S3";
    html += "</div>";

    html += "<div class='info'>";
    html += "<strong>ℹ️ Information:</strong><br>";
    html += "• Current Flash Size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB<br>";
    html += "• Free Sketch Space: " + String(ESP.getFreeSketchSpace() / 1024) + " KB<br>";
    html += "• Chip Model: " + String(ESP.getChipModel()) + "<br>";
    html += "• Current Firmware: " + String(__DATE__) + " " + String(__TIME__);
    html += "</div>";

    html += "<form method='POST' action='/firmware' enctype='multipart/form-data'>";
    html += "<h3>Select Firmware File (.bin)</h3>";
    html += "<input type='file' name='firmware' accept='.bin' required>";
    html += "<br><br>";
    html += "<input type='submit' value='Upload Firmware'>";
    html += "</form>";

    html += "<br>";
    html += "<a href='/' class='back-btn'>← Back to Main Page</a>";
    html += "<a href='/upload' class='back-btn'>File Upload</a>";

    html += "<script>";
    html += "document.querySelector('form').addEventListener('submit', function(e) {";
    html += "  if (!confirm('Are you sure you want to update the firmware? This will reboot the device.')) {";
    html += "    e.preventDefault();";
    html += "  } else {";
    html += "    document.querySelector('input[type=\"submit\"]').value = 'Uploading...';";
    html += "    document.querySelector('input[type=\"submit\"]').disabled = true;";
    html += "  }";
    html += "});";
    html += "</script>";

    html += "</div></body></html>";

    server->send(200, "text/html", html);
}

void WebHandlers::handleDeleteFile()
{
    String filename = server->arg("file");

    if (filename.length() == 0)
    {
        server->send(400, "text/plain", "No filename provided");
        return;
    }

    // Prevent deletion of critical files
    if (filename == "/index.html")
    {
        server->send(403, "text/plain", "Cannot delete index.html - critical system file");
        return;
    }

    if (SPIFFS.exists(filename))
    {
        if (SPIFFS.remove(filename))
        {
            Serial.printf("File deleted: %s\n", filename.c_str());
            server->send(200, "text/plain", "File deleted successfully");
        }
        else
        {
            Serial.printf("Failed to delete file: %s\n", filename.c_str());
            server->send(500, "text/plain", "Failed to delete file");
        }
    }
    else
    {
        server->send(404, "text/plain", "File not found");
    }
}

void WebHandlers::handleUpload()
{
    String html = "<html><head><title>File Upload</title>";
    html += "<style>";
    html += "body { font-family: Arial; text-align: center; margin: 20px; background-color: #f0f0f0; }";
    html += ".container { max-width: 700px; margin: 0 auto; padding: 20px; background-color: white; border-radius: 10px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
    html += "input[type='file'] { margin: 10px; padding: 5px; }";
    html += "input[type='submit'] { background-color: #28a745; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }";
    html += "input[type='submit']:hover { background-color: #218838; }";
    html += ".firmware-btn { background-color: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; text-decoration: none; display: inline-block; margin: 10px; }";
    html += ".firmware-btn:hover { background-color: #0056b3; }";
    html += ".back-btn { background-color: #6c757d; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; text-decoration: none; display: inline-block; margin: 10px; }";
    html += ".back-btn:hover { background-color: #545b62; }";
    html += ".delete-btn { background-color: #dc3545; color: white; padding: 5px 10px; border: none; border-radius: 3px; cursor: pointer; margin-left: 10px; font-size: 12px; }";
    html += ".delete-btn:hover { background-color: #c82333; }";
    html += ".file-list { text-align: left; }";
    html += ".file-item { display: flex; justify-content: space-between; align-items: center; padding: 8px; margin: 5px 0; background-color: #f8f9fa; border-radius: 5px; border: 1px solid #e9ecef; }";
    html += ".file-info { flex-grow: 1; }";
    html += ".warning { background-color: #fff3cd; border: 1px solid #ffeaa7; color: #856404; padding: 10px; margin: 10px 0; border-radius: 5px; font-size: 14px; }";
    html += ".space-info { background-color: #d1ecf1; border: 1px solid #bee5eb; color: #0c5460; padding: 10px; margin: 10px 0; border-radius: 5px; font-size: 14px; }";
    html += "</style></head><body>";

    html += "<div class='container'>";
    html += "<h2>File Management</h2>";

    // SPIFFS space information
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;

    html += "<div class='space-info'>";
    html += "<strong> SPIFFS Storage:</strong><br>";
    html += "Total: " + String(totalBytes / 1024) + " KB | ";
    html += "Used: " + String(usedBytes / 1024) + " KB | ";
    html += "Free: " + String(freeBytes / 1024) + " KB";
    html += "</div>";

    html += "<div class='warning'>";
    html += "<strong> Note:</strong> For firmware updates, use the dedicated firmware update page. ";
    html += "Don't upload .bin files here as they waste SPIFFS space.";
    html += "</div>";

    html += "<h3>Upload File to SPIFFS</h3>";
    html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
    html += "<input type='file' name='upload' required>";
    html += "<br><br>";
    html += "<input type='submit' value='Upload File'>";
    html += "</form>";

    html += "<br>";
    html += "<a href='/firmware' class='firmware-btn'> Update Firmware</a>";
    html += "<a href='/' class='back-btn'>← Back to Main Page</a>";

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
        html += "<div class='file-info'>";
        html += "<strong>" + fileName + "</strong><br>";
        html += "Size: " + String(fileSize) + " bytes";
        if (fileName.endsWith(".bin"))
        {
            html += " <span style='color: #dc3545;'>(Firmware file - consider deleting)</span>";
        }
        html += "</div>";

        // Don't allow deletion of index.html
        if (fileName != "/index.html")
        {
            html += "<button class='delete-btn' onclick='deleteFile(\"" + fileName + "\")'>Delete</button>";
        }
        else
        {
            html += "<span style='color: #6c757d; font-size: 12px;'>Protected</span>";
        }
        html += "</div>";

        file = root.openNextFile();
    }

    if (!hasFiles)
    {
        html += "<div class='file-item'><div class='file-info'>No files found in SPIFFS</div></div>";
    }

    html += "</div>";

    html += "<script>";
    html += "function deleteFile(filename) {";
    html += "  if (confirm('Are you sure you want to delete ' + filename + '?')) {";
    html += "    fetch('/delete?file=' + encodeURIComponent(filename), { method: 'POST' })";
    html += "      .then(response => response.text())";
    html += "      .then(data => {";
    html += "        alert(data);";
    html += "        location.reload();";
    html += "      })";
    html += "      .catch(error => {";
    html += "        alert('Error deleting file: ' + error);";
    html += "      });";
    html += "  }";
    html += "}";
    html += "</script>";

    html += "</div></body></html>";

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
               { server->send(200, "text/plain", "Upload complete"); }, [this]()
               { this->handleFileUpload(); });
    server->on("/firmware", HTTP_GET, [this]()
               { this->handleFirmwareUpdate(); });
    server->on("/firmware", HTTP_POST, [this]()
               { 
                   server->send(200, "text/plain", "Firmware update complete. Device will reboot in 3 seconds...");
                   delay(3000);
                   ESP.restart(); }, [this]()
               { this->handleFirmwareUpload(); });
    server->on("/delete", HTTP_POST, [this]()
               { this->handleDeleteFile(); });
}