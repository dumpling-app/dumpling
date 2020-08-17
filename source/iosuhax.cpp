#include "iosuhax.h"

int fsaHandle = -1;
int iosuhaxHandle = -1;
int mcpHookHandle = -1;

void haxchiCallback(IOSError err, void* dummy) {
    return;
}

bool openMCPHook() {
    // Take over mcp thread
    mcpHookHandle = MCP_Open();
    if (mcpHookHandle < 0) return false;

    IOSError err = IOS_IoctlAsync(mcpHookHandle, 0x62, NULL, 0, NULL, 0, haxchiCallback, NULL);
    if (err != IOS_ERROR_OK) {
        MCP_Close(mcpHookHandle);
        mcpHookHandle = -1;
        return false;
    }
    OSSleepTicks(OSSecondsToTicks(1));

    if (IOSUHAX_Open("/dev/mcp") < 0) {
        MCP_Close(mcpHookHandle);
        mcpHookHandle = -1;
        return false;
    }
    return true;
}

void closeMCPHook() {
    if (mcpHookHandle < 0) return;
    // close down wupserver, return control to mcp
    IOSUHAX_Close();
    OSSleepTicks(OSSecondsToTicks(1));

    MCP_Close(mcpHookHandle);
    mcpHookHandle = -1;
}

bool openIosuhax() {
    iosuhaxHandle = IOSUHAX_Open(NULL);
    if (iosuhaxHandle < 0) {
        if (!openMCPHook()) {
            WHBLogPrint("Couldn't open iosuhax!");
            WHBLogPrint("You should launch Mocha first before starting this app!");
            return false;
        }
    }

    fsaHandle = IOSUHAX_FSA_Open();
    if (fsaHandle < 0) {
        WHBLogPrint("Couldn't open iosuhax_fsa!");
        return false;
    }
    return true;
}

void closeIosuhax() {
    IOSUHAX_Close();
    closeMCPHook();
    IOSUHAX_FSA_Close(fsaHandle);
}

int getFSAHandle() {
    return fsaHandle;
}

int getIosuhaxHandle() {
    return iosuhaxHandle;
}