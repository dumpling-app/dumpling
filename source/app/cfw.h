#include "common.h"
#include <mocha/mocha.h>


enum CFWVersion {
    FAILED,
    NONE,
    MOCHA_FSCLIENT,
    CUSTOM_MOCHA,
    DUMPLING,
    CEMU,
};

CFWVersion testCFW();
bool initCFW();
void shutdownCFW();
CFWVersion getCFWVersion();