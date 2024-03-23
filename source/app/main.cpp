#include "menu.h"
#include "navigation.h"
#include "cfw.h"
#include "filesystem.h"
#include "exploit.h"
#include "titles.h"
#include "users.h"
#include "gui.h"
#include "http.h"

// Initialize correct heaps for CustomRPXLoader
extern "C" void __init_wut_malloc();
extern "C" [[maybe_unused]] void __preinit_user(MEMHeapHandle *outMem1, MEMHeapHandle *outFG, MEMHeapHandle *outMem2) {
    __init_wut_malloc();
}

int main() {
    // Initialize libraries
    initializeGUI();
    FSInit();
    FSAInit();
    nn::act::Initialize();
    ACPInitialize();
    initializeInputs();
    http_init();

    IMDisableAPD(); // Disable auto-shutdown feature

    // Start Dumpling
    showLoadingScreen();
    if (testCFW() != FAILED && ((getCFWVersion() == MOCHA_FSCLIENT || getCFWVersion() == CEMU) || executeExploit()) && initCFW() && mountSystemDrives() && loadUsers() && loadTitles(true)) {
        WHBLogFreetypePrint(L"");
        WHBLogPrint("Finished loading!");
        WHBLogFreetypeDraw();
        sleep_for(3s);
        showMainMenu();
    }

    WHBLogPrint("");
    WHBLogPrint(getCFWVersion() == MOCHA_FSCLIENT ? "Exiting Dumpling..." : "Exiting Dumpling and shutting off Wii U...");
    WHBLogFreetypeDraw();
    sleep_for(5s);

    // Close application properly
    http_exit();
    unmountSystemDrives();
    shutdownCFW();
    ACPFinalize();
    nn::act::Finalize();
    FSShutdown();
    VPADShutdown();
    shutdownGUI();

    exitApplication(getCFWVersion() != MOCHA_FSCLIENT && USE_DEBUG_STUBS == 0);
}