#ifndef FILESYSTEM_UTILS_H
#define FILESYSTEM_UTILS_H

#include <Arduino.h>
#include <SPIFFS.h>

class FilesystemUtils
{
public:
    static bool initSPIFFS();
    static void listFiles();
    static bool checkIndexFile();
    static void printFileInfo(const String &filename);
    static bool deleteFile(const String &filename);
    static bool fileExists(const String &filename);
    static size_t getFileSize(const String &filename);
    static void formatSPIFFS();
};

#endif