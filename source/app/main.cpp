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
    initializeInputs();

    IMDisableAPD(); // Disable auto-shutdown feature

    // Start Dumpling
    showLoadingScreen();
    CFWVersion iosuhaxCFW = testIosuhax();
    if ((iosuhaxCFW == CFWVersion::TIRAMISU_RPX || executeExploit()) && openIosuhax() && mountSystemDrives() && loadUsers() && loadTitles(true)) {
        WHBLogPrint("");
        WHBLogPrint("Finished loading!");
        WHBLogConsoleDraw();
        OSSleepTicks(OSSecondsToTicks(3));
        showMainMenu();
    }

    WHBLogPrint("");
    WHBLogPrint(iosuhaxCFW == CFWVersion::TIRAMISU_RPX ? "Exiting Dumpling..." : "Exiting Dumpling and shutting off Wii U...");
    WHBLogConsoleDraw();
    OSSleepTicks(OSSecondsToTicks(5));

    // Close application properly
    unmountSD();
    unmountUSBDrive();
    unmountSystemDrives();
    closeIosuhax();
    nn::act::Finalize();
    FSShutdown();
    VPADShutdown();
    shutdownGUI();

    exitApplication(iosuhaxCFW != CFWVersion::TIRAMISU_RPX);
}