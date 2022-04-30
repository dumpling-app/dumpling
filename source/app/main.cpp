#include "menu.h"
#include "navigation.h"
#include "iosuhax.h"
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
    CFWVersion iosuhaxCFW = testIosuhax();
    if (iosuhaxCFW != FAILED && ((iosuhaxCFW == TIRAMISU_RPX || iosuhaxCFW == CEMU) || executeExploit()) && openIosuhax() && mountSystemDrives() && loadUsers() && loadTitles(true)) {
        WHBLogPrint("");
        WHBLogPrint("Finished loading!");
        WHBLogFreetypeDraw();
        sleep_for(2s);
        showMainMenu();
    }

    WHBLogPrint("");
    WHBLogPrint(iosuhaxCFW == TIRAMISU_RPX ? "Exiting Dumpling..." : "Exiting Dumpling and shutting off Wii U...");
    WHBLogFreetypeDraw();
    sleep_for(5s);

    // Close application properly
    unmountSD();
    unmountUSBDrive();
    unmountSystemDrives();
    closeIosuhax();
    ACPFinalize();
    nn::act::Finalize();
    FSShutdown();
    VPADShutdown();
    shutdownGUI();

    exitApplication(iosuhaxCFW != TIRAMISU_RPX);
}