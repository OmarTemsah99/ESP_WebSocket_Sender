#ifndef FILESYSTEM_UTILS_H
#define FILESYSTEM_UTILS_H

#include <SPIFFS.h>

class FilesystemUtils
{
public:
    static bool initSPIFFS();
    static void listFiles();
    static bool checkIndexFile();
    static void printFileInfo(const String &filename);
};

#endif // FILESYSTEM_UTILS_H