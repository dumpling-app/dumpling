#include "iosuhax.h"

int fsaHandle = -1;
int iosuhaxHandle = -1;
int mcpHookHandle = -1;

void someFunc(IOSError err, void* arg) {
    (void)arg;
}

bool openMCPHook() {
    // Take over mcp thread
    mcpHookHandle = MCP_Open();
    if (mcpHookHandle < 0) return false;

    IOS_IoctlAsync(mcpHookHandle, 0x62, (void*)0, 0, (void*)0, 0, someFunc, (void*)0);
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