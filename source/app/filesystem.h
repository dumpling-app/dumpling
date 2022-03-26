#include "common.h"

// Functions related to devices
bool mountSystemDrives();
bool mountSD();
bool mountUSBDrive();
bool mountDisc();
bool unmountSystemDrives();
void unmountSD();
void unmountUSBDrive();
bool unmountDisc();

bool isSDMounted();
bool isUSBDriveMounted();
bool isDiscMounted();
bool isExternalStorageMounted();
bool testStorage(titleLocation location);
bool isDiscInserted();
bool isSDInserted();
bool isUSBDriveInserted();

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
titleLocation deviceToLocation(const char* device);
titleLocation pathToLocation(const char* device);