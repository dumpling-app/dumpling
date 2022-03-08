#include "common.h"
#include <iosuhax.h>
#include <iosuhax_disc_interface.h>

enum CFWVersion {
    FAILED,
    NONE,
    MOCHA_NORMAL,
    TIRAMISU_RPX,
    MOCHA_LITE,
    HAXCHI,
    HAXCHI_MCP,
    DUMPLING,
};

struct MCPVersion {
    int major;
    int minor;
    int patch;
    char region[4];
};

#define IOSUHAX_MAGIC_WORD 0x4E696365

CFWVersion testIosuhax();
bool openIosuhax();
void closeIosuhax();
int32_t getFSAHandle();
int32_t getIosuhaxHandle();
CFWVersion getCFWVersion();