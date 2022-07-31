#include "common.h"
#include <mocha/mocha.h>


enum CFWVersion {
    FAILED,
    NONE,
    MOCHA_FSCLIENT,
    DUMPLING,
    CEMU,
};

CFWVersion testCFW();
bool initCFW();
void shutdownCFW();
CFWVersion getCFWVersion();