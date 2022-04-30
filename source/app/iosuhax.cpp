#include "iosuhax.h"
#include "gui.h"
#include "menu.h"
#include "navigation.h"

int32_t fsaHandle = -1;
int32_t iosuhaxHandle = -1;
CFWVersion currCFWVersion = CFWVersion::NONE;

bool stopTiramisuServer() {
    WHBLogFreetypeClear();
    WHBLogPrint("Opening iosuhax to send stop command...");
    WHBLogFreetypeDraw();

    IOSHandle iosuhaxHandle = IOS_Open("/dev/iosuhax", (IOSOpenMode)0);
    if (iosuhaxHandle < IOS_ERROR_OK) {
        WHBLogPrint("Couldn't open /dev/iosuhax to stop Tiramisu?");
        WHBLogFreetypeDraw();
        return false;
    }

    WHBLogPrint("Sending stop command to Tiramisu... ");
    WHBLogFreetypeDraw();
    sleep_for(250ms);

    alignas(0x20) int32_t responseBuffer[0x20 >> 2];
    *responseBuffer = 0;
    IOS_Ioctl(iosuhaxHandle, 0x03/*IOCTL_KILL_SERVER*/, nullptr, 0, responseBuffer, 4);
    
    WHBLogPrint("Waiting for Tiramisu to stop... ");
    WHBLogFreetypeDraw();
    sleep_for(2s);
    return true;
}

CFWVersion testIosuhax() {
    WHBLogPrint("Detecting prior iosuhax version...");
    WHBLogFreetypeDraw();

    IOSHandle mcpHandle = IOS_Open("/dev/mcp", (IOSOpenMode)0);
    if (mcpHandle < IOS_ERROR_OK) {
        WHBLogPrint("Couldn't open MCP when testing for iosuhax!");
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
            uint8_t enableWiiMode = showDialogPrompt("Do you want to enable Wii game dumping support?\n\nIf yes, Tiramisu CFW lacks some stuff required for reading Wii files.\nWhile work is being done to add it, you can still\nenable support but Dumpling has to stop Tiramisu temporarily.", "Enable Wii support", "Just use Tiramisu CFW");
            if (enableWiiMode == 0) {
                if (stopTiramisuServer()) {
                    WHBLogFreetypeClear();
                    WHBLogPrint("Detected and stopped Tiramisu...");
                    WHBLogPrint("Attempt to replace it with Dumpling CFW...");
                    currCFWVersion = CFWVersion::NONE;
                }
                else {
                    WHBLogFreetypeClear();
                    WHBLogPrint("Failed to stop Tiramisu CFW!");
                    WHBLogPrint("You can temporarily undo Tiramisu autobooting:");
                    WHBLogPrint("https://wiiu.hacks.guide/#/uninstall-payloadloader");
                    WHBLogPrint("Only follow the undo autobooting section!");
                    WHBLogPrint("");
                    WHBLogPrint("You can then start Dumpling by going to");
                    WHBLogPrint("https://dumplingapp.com in the browser");
                    WHBLogPrint("");
                    WHBLogPrint("Exiting Dumpling in 10 seconds...");
                    WHBLogFreetypeDraw();
                    sleep_for(10s);
                    currCFWVersion = CFWVersion::FAILED;
                }
            }
            else {
                WHBLogPrint("Detected Tiramisu CFW...");
                WHBLogPrint("Will use Tiramisu's iosuhax implementation!");
                currCFWVersion = CFWVersion::TIRAMISU_RPX;
            }
            return currCFWVersion;
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