#include "common.h"

// Functions related to devices
bool mountSystemDrives();
bool mountSD();
bool mountUSBDrives();
bool mountDisc();
bool unmountSystemDrives();
void unmountUSBDrives();
void unmountSD();
void unmountUSBDrives();
bool unmountDisc();

bool isUSBInserted();
bool isDiscInserted();
bool isExternalStorageInserted();

// Filesystem helper functions
std::string convertToPosixPath(const char* volPath);
std::string convertToDevicePath(const char* volPath);
bool fileExist(const char* path);
bool dirExist(const char* path);
bool isDirEmpty(const char* path);
void createPath(const char* path);
std::string formatByteSize(uint64_t bytes);
std::string formatByteSizes(uint64_t dominantBytes, uint64_t otherBytes);
uint64_t getFreeSpace(const char* path);
uint64_t getUsedSpace(const char* path);
std::string getRootFromLocation(dumpLocation location);
dumpLocation getLocationFromRoot(std::string rootPath);
titleLocation deviceToLocation(char* device);
