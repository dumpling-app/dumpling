#include "common.h"

// Functions related to devices
bool mountSystemDrives();
bool mountDisc();
bool unmountSystemDrives();
bool unmountDisc();

bool isDiscMounted();
bool testStorage(TITLE_LOCATION location);

// Filesystem helper functions
std::string convertToPosixPath(const char* volPath);
bool dirExist(const char* path);

TITLE_LOCATION deviceToLocation(const char* device);
TITLE_LOCATION pathToLocation(const char* device);