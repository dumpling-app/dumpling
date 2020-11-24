#include "iosuhax.h"
#include "gui.h"

int32_t mcpHookHandle = -1;
int32_t fsaHandle = -1;
int32_t iosuhaxHandle = -1;

OSEvent haxStartEvent = {};

void haxStartCallback(IOSError arg1, void *arg2) {
}

bool openIosuhax() {
    WHBLogPrint("Preparing iosuhax...");
    WHBLogConsoleDraw();
    
    // Open MCP to send the start command
    mcpHookHandle = MCP_Open();
    if (mcpHookHandle < 0) {
        WHBLogPrint("Failed to open the MCP IPC!");
        return false;
    }

    // Send 0x62 ioctl command that got replaced in the ios_kernel to run the wupserver
    IOS_IoctlAsync(mcpHookHandle, 0x62, nullptr, 0, nullptr, 0, haxStartCallback, nullptr);
    OSSleepTicks(OSSecondsToTicks(1));

    // Connect to dumplinghax
    iosuhaxHandle = IOSUHAX_Open("/dev/mcp");
    if (iosuhaxHandle < 0) {
        WHBLogPrint("Couldn't open iosuhax :/");
        WHBLogPrint("Something interfered with the exploit...");
        WHBLogPrint("Try restarting your Wii U and launching Dumpling again!");
        return false;
    }

    fsaHandle = IOSUHAX_FSA_Open();
    if (fsaHandle < 0) {
        WHBLogPrint("Couldn't open iosuhax FSA!");
        return false;
    }
    return true;
}

void closeIosuhax() {
    if (fsaHandle > 0) IOSUHAX_FSA_Close(fsaHandle);
    if (iosuhaxHandle > 0) IOSUHAX_Close();
    if (mcpHookHandle > 0) MCP_Close(mcpHookHandle);
    OSSleepTicks(OSSecondsToTicks(1));
    mcpHookHandle = -1;
    fsaHandle = -1;
    iosuhaxHandle = -1;
}

int32_t getFSAHandle() {
    return fsaHandle;
}

int32_t getIosuhaxHandle() {
    return iosuhaxHandle;
}