#include "menu.h"
#include "navigation.h"
#include "cfw.h"
#include "filesystem.h"
#include "exploit.h"
#include "memory.h"
#include "titles.h"
#include "users.h"
#include "gui.h"

int main() {
    // Initialize libraries
    initializeGUI();
    FSInit();
    nn::act::Initialize();
    ACPInitialize();
    initializeInputs();

    IMDisableAPD(); // Disable auto-shutdown feature

    // Start Dumpling
    showLoadingScreen();
    if (testCFW() != FAILED && ((getCFWVersion() == MOCHA_FSCLIENT || getCFWVersion() == CEMU) || executeExploit()) && initCFW() && mountSystemDrives() && loadUsers() && loadTitles(true)) {
        WHBLogPrint("");
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
    unmountSD();
    unmountUSBDrive();
    unmountSystemDrives();
    shutdownCFW();
    ACPFinalize();
    nn::act::Finalize();
    FSShutdown();
    VPADShutdown();
    shutdownGUI();

    exitApplication(getCFWVersion() != MOCHA_FSCLIENT);
}