#include "common.h"

// Mount Indicators
extern bool mlcMounted;
extern bool usbMounted;
extern bool discMounted;
extern bool sdfatMounted;
extern bool usbfatMounted;

// Functions related to devices
bool mountDevices();
bool mountDisc();
bool unmountDevices();
bool unmountDisc();

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
