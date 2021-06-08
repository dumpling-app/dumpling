#include "menu.h"
#include "navigation.h"
#include "iosuhax.h"
#include "filesystem.h"
#include "exploit.h"
#include "memory.h"
#include "titles.h"
#include "users.h"
#include "gui.h"

static OSDynLoad_Module coreinitHandle = nullptr;
static int32_t (*OSShutdown)(int32_t status);
static void (*OSForceFullRelaunch)();

int main() {
    // Initialize libraries
    initializeGUI();
    FSInit();
    nn::act::Initialize();
    initializeInputs();

    IMDisableAPD(); // Disable auto-shutdown feature

    // Initialize OSShutdown and OSForceFullRelaunch functions
    OSDynLoad_Acquire("coreinit.rpl", &coreinitHandle);
    OSDynLoad_FindExport(coreinitHandle, FALSE, "OSShutdown", (void **)&OSShutdown);
    OSDynLoad_FindExport(coreinitHandle, FALSE, "OSForceFullRelaunch", (void **)&OSForceFullRelaunch);
    OSDynLoad_Release(coreinitHandle);

    // Start Dumpling
    showLoadingScreen();
    if (getTitles() && executeExploit() && openIosuhax() && mountSystemDrives() && mountSD() && loadUsers() && loadTitles(true)) {
        showMainMenu();
    }

    WHBLogPrint("");
    WHBLogPrint("Exiting Dumpling and shutting off the console...");
    WHBLogConsoleDraw();
    OSSleepTicks(OSSecondsToTicks(5));

    // Prevent shutdown when some debugging keys are pressed
    bool shutdownOnExit = false; // todo: change back
    if (navigatedUp() && pressedStart()) shutdownOnExit = false;

    // Close application properly
    unmountSD();
    unmountUSBDrives();
    unmountSystemDrives();
    closeIosuhax();
    nn::act::Finalize();
    FSShutdown();
    VPADShutdown();
    shutdownGUI();
    
    if (shutdownOnExit) OSShutdown(1);
    else SYSRelaunchTitle(0, NULL);
}