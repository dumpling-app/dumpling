#include <coreinit/filesystem.h>
#include <coreinit/screen.h>
#include <whb/proc.h>
#include <coreinit/energysaver.h>
#include <nn/acp.h>
#include <nn/ac.h>

#include "menu.h"
#include "navigation.h"
#include "iosuhax.h"
#include "filesystem.h"
#include "exploit.h"
#include "memory.h"
#include "titles.h"
#include "users.h"

static OSDynLoad_Module coreinitHandle = NULL;
static void *sOSForceFullRelaunch = NULL;

int32_t OSForceFullRelaunch() {
   return reinterpret_cast<decltype(&OSForceFullRelaunch)>(sOSForceFullRelaunch)();
}

int main() {
    // Initialize libraries
    WHBProcInit();
    WHBLogConsoleInit();
    FSInit();
    nn::act::Initialize();
    initializeInputs();

    IMDisableAPD(); // Disable auto-shutdown feature

    // Initialize force OS restart function
    OSDynLoad_Acquire("coreinit.rpl", &coreinitHandle);
    OSDynLoad_FindExport(coreinitHandle, FALSE, "OSForceFullRelaunch", &sOSForceFullRelaunch);
    OSDynLoad_Release(coreinitHandle);

    // Setup environment to dump discs
    showLoadingScreen();
    if (getTitles() && executeExploit() && openIosuhax() && mountSystemDrives() && mountSD() && loadUsers() && loadTitles(true)) {
        showMainMenu();
    }

    WHBLogPrint("");
    WHBLogPrint("Exiting Dumpling...");
    WHBLogPrint("Will fully restart OS to the home menu!");
    WHBLogConsoleDraw();
    OSSleepTicks(OSSecondsToTicks(5));

    // Close application properly
    unmountSD();
    unmountUSBDrives();
    unmountSystemDrives();
    closeIosuhax();
    WHBProcShutdown();
    WHBLogConsoleFree();
    nn::act::Finalize();
    FSShutdown();
    VPADShutdown();
    SYSLaunchMenu();
    OSForceFullRelaunch();
    return 0;
}