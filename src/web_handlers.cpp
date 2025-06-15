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

void WebHandlers::handleUpload()
{
    String html = "<html><body>";
    html += "<h2>File Upload</h2>";
    html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
    html += "<input type='file' name='upload'>";
    html += "<input type='submit' value='Upload'>";
    html += "</form>";
    html += "<h3>Current Files:</h3><ul>";

    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
        String fileName = file.name();
        size_t fileSize = file.size();
        html += "<li>" + String(fileName) + " - " + String(fileSize) + " bytes</li>";
        file = root.openNextFile();
    }
    html += "</ul></body></html>";

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
}