#ifndef FILESYSTEMMANAGER_H
#define FILESYSTEMMANAGER_H

#include <Arduino.h>

class FileSystemManager {
public:
  void setup();
  String readFile(const char* path);
};

extern FileSystemManager fileSystemManager;

#endif // FILESYSTEMMANAGER_H
