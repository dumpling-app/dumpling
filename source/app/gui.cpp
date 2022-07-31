#include "gui.h"
#include "../font/log_freetype.h"
#include "navigation.h"

static bool usingHBL = false;

#define HBL_TITLE_ID (0x0005000013374842)
#define MII_MAKER_JPN_TITLE_ID (0x000500101004A000)
#define MII_MAKER_USA_TITLE_ID (0x000500101004A100)
#define MII_MAKER_EUR_TITLE_ID (0x000500101004A200)

void saveProcessCallback() {
    OSSavesDone_ReadyToRelease();
}

bool initializeGUI() {
    // Setup proc UI
    uint64_t titleId = OSGetTitleID();
    if (titleId == HBL_TITLE_ID || titleId == MII_MAKER_USA_TITLE_ID || titleId == MII_MAKER_EUR_TITLE_ID || titleId == MII_MAKER_JPN_TITLE_ID) {
        usingHBL = true;
    }

    WHBLogFreetypeInit();
    
    OSEnableHomeButtonMenu(false);
    ProcUIInit(&saveProcessCallback);
    return true;
}

void shutdownGUI() {
    WHBLogFreetypeFree();
}

void exitApplication(bool shutdownOnExit) {
    // Loop through ProcUI messages until it says Dumpling should exit
    ProcUIStatus status;
    while((status = ProcUIProcessMessages(true)) != PROCUI_STATUS_EXITING) {
        if (status == PROCUI_STATUS_RELEASE_FOREGROUND) {
            ProcUIDrawDoneRelease();
        }

        if (status != PROCUI_STATUS_IN_FOREGROUND) {
            continue;
        }

        if (shutdownOnExit) OSShutdown();
        else if (usingHBL) SYSRelaunchTitle(0, NULL);
        else SYSLaunchMenu();
    }
    ProcUIShutdown();
}