#include "iosuhax.h"
#include "gui.h"

int32_t fsaHandle = -1;
int32_t iosuhaxHandle = -1;
CFWVersion currCFWVersion = CFWVersion::NONE;

CFWVersion testIosuhax() {
    WHBLogPrint("Detecting prior iosuhax version...");
    WHBLogFreetypeDraw();

    IOSHandle mcpHandle = IOS_Open("/dev/mcp", (IOSOpenMode)0);
    if (mcpHandle < IOS_ERROR_OK) {
        WHBLogPrint("Couldn't open MCP!!");
        WHBLogFreetypeDraw();
        sleep_for(5s);
        return CFWVersion::FAILED;
    }

    IOSHandle iosuhaxHandle = IOS_Open("/dev/iosuhax", (IOSOpenMode)0);
    // Cemu triggers this
    if (iosuhaxHandle >= IOS_ERROR_OK) {
        // Close iosuhax
        IOS_Close(iosuhaxHandle);

        // Test behavior of Mocha version by sending a subcommand to IOCTL100 to see which implementation is underneath
        constexpr int32_t dummyBufferSize = 0x300;
        void* dummyBuffer = memalign(0x20, 0x300);
        *(uint32_t*)dummyBuffer = 0;
        int32_t returnValue = 0;
        IOS_Ioctl(mcpHandle, 100, dummyBuffer, dummyBufferSize, &returnValue, sizeof(returnValue));
        free(dummyBuffer);
        IOS_Close(mcpHandle);

        if (returnValue == 1) {
            WHBLogPrint("Detected Tiramisu CFW...");
            WHBLogPrint("Will use Tiramisu's iosuhax implementation!");
            currCFWVersion = CFWVersion::TIRAMISU_RPX;
        }
        else if (returnValue == 2) {
            WHBLogPrint("Detected MochaLite CFW");
            WHBLogPrint("Attempt to replace it with Dumpling CFW...");
            currCFWVersion = CFWVersion::MOCHA_LITE;
        }
        else if (returnValue == 3) {
            WHBLogPrint("Detected Dumpling CFW");
            WHBLogPrint("Attempt to replace it with Dumpling CFW...");
            currCFWVersion = CFWVersion::DUMPLING;
        }
        else {
            WHBLogPrint("Detected Mocha CFW");
            WHBLogPrint("Attempt to replace it with Dumpling CFW...");
            currCFWVersion = CFWVersion::MOCHA_NORMAL;
        }
        return currCFWVersion;
    }

    // Test if haxchi FW is used with MCP hook
    uint32_t* returnValue = (uint32_t*)memalign(0x20, 0x100);
    IOS_Ioctl(mcpHandle, 0x5B, NULL, 0, returnValue, sizeof(*returnValue));
    if (*returnValue == IOSUHAX_MAGIC_WORD) {
        free(returnValue);
        WHBLogPrint("Detected Haxchi CFW with MCP hook");
        WHBLogPrint("Attempt to replace it with Dumpling CFW...");
        IOS_Close(mcpHandle);
        currCFWVersion = CFWVersion::HAXCHI_MCP;
        return CFWVersion::HAXCHI_MCP;
    }
    free(returnValue);

    MCPVersion* mcpVersion = (MCPVersion*)memalign(0x20, 0x100);
    IOS_Ioctl(mcpHandle, 0x89, NULL, 0, mcpVersion, sizeof(*mcpVersion));
    if (mcpVersion->major == 99 && mcpVersion->minor == 99 && mcpVersion->patch == 99) {
        free(mcpVersion);
        WHBLogPrint("Detected Haxchi CFW without MCP hook");
        WHBLogPrint("Attempt to replace it with Dumpling CFW...");
        IOS_Close(mcpHandle);
        currCFWVersion = CFWVersion::HAXCHI;
        return CFWVersion::HAXCHI;
    }
    free(mcpVersion);

    if (IOS_Open("/dev/nonexistent", (IOSOpenMode)0) == 0x1) {
        WHBLogPrint("Running under Cemu!");
        WHBLogPrint("Use stubs to immitate CFW...");
        IOS_Close(mcpHandle);
        currCFWVersion = CFWVersion::CEMU;
        return CFWVersion::CEMU;
    }

    WHBLogPrint("No CFW detected!");
    WHBLogPrint("Run Dumpling CFW for iosuhax access...");
    IOS_Close(mcpHandle);
    currCFWVersion = CFWVersion::NONE;
    return CFWVersion::NONE;
}

bool openIosuhax() {
    WHBLogPrint("Preparing iosuhax...");
    WHBLogFreetypeDraw();

    if (getCFWVersion() == CFWVersion::CEMU) return true;

    // Connect to iosuhax
    iosuhaxHandle = IOSUHAX_Open(NULL);
    if (iosuhaxHandle < 0) {
        WHBLogPrint("Couldn't open /dev/iosuhax :/");
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
    sleep_for(1s);
    fsaHandle = -1;
    iosuhaxHandle = -1;
}

int32_t getFSAHandle() {
    return fsaHandle;
}

int32_t getIosuhaxHandle() {
    return iosuhaxHandle;
}

CFWVersion getCFWVersion() {
    return currCFWVersion;
}