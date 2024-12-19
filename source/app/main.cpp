#include "menu.h"
#include "navigation.h"
#include "cfw.h"
#include "filesystem.h"
#include "exploit.h"
#include "titles.h"
#include "users.h"
#include "gui.h"

// Initialize correct heaps for CustomRPXLoader
extern "C" void __init_wut_malloc();
extern "C" [[maybe_unused]] void __preinit_user(MEMHeapHandle *outMem1, MEMHeapHandle *outFG, MEMHeapHandle *outMem2) {
    __init_wut_malloc();
}

int main() {
    // Initialize libraries
    initializeGUI();
    WHBLogCafeInit();
    FSInit();
    FSAInit();
    nn::act::Initialize();
    ACPInitialize();
    initializeInputs();

    IMDisableAPD(); // Disable auto-shutdown feature

    // Start Dumpling
    showLoadingScreen();
    if (testCFW() != FAILED && ((getCFWVersion() == MOCHA_FSCLIENT || getCFWVersion() == CEMU || getCFWVersion() == CUSTOM_MOCHA) || executeExploit()) && initCFW() && mountSystemDrives() && loadUsers() && loadTitles(true)) {
        WHBLogFreetypePrint(L"");
        WHBLogPrint("Finished loading!");
        WHBLogFreetypeDraw();
        sleep_for(3s);
        showMainMenu();
    }

    WHBLogPrint("");
    WHBLogPrint(getCFWVersion() == MOCHA_FSCLIENT ? "Exiting Dumpling..." : "Exiting Dumpling and rebooting Wii U...");
    WHBLogFreetypeDraw();
    sleep_for(5s);

    // Close application properly
    unmountSystemDrives();
    shutdownCFW();
    ACPFinalize();
    nn::act::Finalize();
    FSShutdown();
    VPADShutdown();
    shutdownGUI();

    exitApplication(getCFWVersion() != MOCHA_FSCLIENT && USE_DEBUG_STUBS == 0);
}