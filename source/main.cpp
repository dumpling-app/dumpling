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


int main(int argc, char **argv) {
    // Initialize libraries
    WHBProcInit();
    WHBLogConsoleInit();
    FSInit();
    nn::act::Initialize();
    initializeInputs();

    IMDisableAPD(); // Disable auto-shutdown feature

    // Setup environment to dump discs
    if (openIosuhax() && mountDevices() && showLoadingScreen()) {
        showMainMenu();
    }

    WHBLogPrint("");
    WHBLogPrint("Exiting dumpling...");
    WHBLogConsoleDraw();
    OSSleepTicks(OSSecondsToTicks(1));

    // Close application properly

    unmountDevices();
    closeIosuhax();
    WHBProcShutdown();
    WHBLogConsoleFree();
    FSShutdown();
    VPADShutdown();
    nn::act::Finalize();
}