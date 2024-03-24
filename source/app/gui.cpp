#include "gui.h"
#include "navigation.h"
#include "cfw.h"

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
    if (uint64_t titleId = OSGetTitleID(); titleId == HBL_TITLE_ID || titleId == MII_MAKER_USA_TITLE_ID || titleId == MII_MAKER_EUR_TITLE_ID || titleId == MII_MAKER_JPN_TITLE_ID) {
        usingHBL = true;
    }

    if (WHBLogFreetypeInit()) {
        OSFatal("Couldn't initialize the GUI properly due to WHBLogFreetypeInit failing!\n");
        return false;
    }
    
    OSEnableHomeButtonMenu(false);
    ProcUIInit(&saveProcessCallback);
    return true;
}

void shutdownGUI() {
    WHBLogFreetypeFree();
}

bool stillRunning() {
    switch (ProcUIProcessMessages(true)) {
        case PROCUI_STATUS_EXITING: {
            return false;
        }
        case PROCUI_STATUS_RELEASE_FOREGROUND: {
            ProcUIDrawDoneRelease();
            break;
        }
        case PROCUI_STATUS_IN_FOREGROUND: {
            break;
        }
        case PROCUI_STATUS_IN_BACKGROUND:
        default:
            break;
    }
    return true;
}

void exitApplication(bool shutdownOnExit) {
    // Loop through ProcUI messages until it says Dumpling should exit
    if (getCFWVersion() == MOCHA_FSCLIENT) {
        SYSLaunchMenu();

        while(stillRunning()) {
            sleep_for(100ms);
        }
    }
    else {
        ProcUIStatus status;
        while((status = ProcUIProcessMessages(true)) != PROCUI_STATUS_EXITING) {
            if (status == PROCUI_STATUS_RELEASE_FOREGROUND) {
                ProcUIDrawDoneRelease();
            }

            if (status != PROCUI_STATUS_IN_FOREGROUND) {
                continue;
            }

            if (shutdownOnExit) OSShutdown();
            else if (usingHBL) SYSRelaunchTitle(0, nullptr);
        }
    }
    ProcUIShutdown();
}