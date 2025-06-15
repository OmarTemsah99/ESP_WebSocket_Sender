#include "filesystem_utils.h"

bool FilesystemUtils::initSPIFFS()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS Mount Failed. Trying to format...");
        if (!SPIFFS.format())
        {
            Serial.println("SPIFFS Format Failed");
            return false;
        }
        if (!SPIFFS.begin())
        {
            Serial.println("SPIFFS Mount Failed after formatting");
            return false;
        }
    }
    Serial.println("SPIFFS mounted successfully");
    return true;
}

void FilesystemUtils::listFiles()
{
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    Serial.println("\nFiles in SPIFFS:");

    while (file)
    {
        String fileName = file.name();
        size_t fileSize = file.size();
        Serial.printf("- File: %s, Size: %d bytes\n", fileName.c_str(), fileSize);
        file = root.openNextFile();
    }
    root.close();
}

bool FilesystemUtils::checkIndexFile()
{
    if (SPIFFS.exists("/index.html"))
    {
        File indexFile = SPIFFS.open("/index.html", "r");
        if (indexFile)
        {
            Serial.printf("index.html found, size: %d bytes\n", indexFile.size());
            indexFile.close();
            return true;
        }
        else
        {
            Serial.println("index.html exists but cannot be opened");
            return false;
        }
    }
    else
    {
        Serial.println("index.html not found in SPIFFS");
        return false;
    }
}

void FilesystemUtils::printFileInfo(const String &filename)
{
    if (SPIFFS.exists(filename))
    {
        File file = SPIFFS.open(filename, "r");
        if (file)
        {
            Serial.printf("File: %s, Size: %d bytes\n", filename.c_str(), file.size());
            file.close();
        }
        else
        {
            Serial.printf("File %s exists but cannot be opened\n", filename.c_str());
        }
    }
    else
    {
        Serial.printf("File %s not found\n", filename.c_str());
    }
}